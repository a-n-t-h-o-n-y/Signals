#ifndef SIGNALS_DETAIL_SIGNAL_IMPL_HPP
#define SIGNALS_DETAIL_SIGNAL_IMPL_HPP
#include <signals/connection.hpp>
#include <signals/detail/connection_impl.hpp>
#include <signals/detail/slot_iterator.hpp>
#include <signals/position.hpp>

#include <cstddef>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <utility>

namespace sig {

// Forward Declaration
template <typename Signature>
class Connection_impl;

// Forward declaration, so that Signature can be split into Ret and Args...
template <typename Signature,
          typename Combiner,
          typename Group,
          typename Group_compare,
          typename Slot_function,
          typename Mutex>
class Signal_impl;

template <typename Ret,
          typename... Args,
          typename Combiner,
          typename Group,
          typename Group_compare,
          typename Slot_function,
          typename Mutex>
class Signal_impl<Ret(Args...),
                  Combiner,
                  Group,
                  Group_compare,
                  Slot_function,
                  Mutex> {
   public:
    using Result_type   = typename Combiner::Result_type;
    using Signature     = Ret(Args...);
    using Slot_type     = Slot<Signature, Slot_function>;
    using Extended_slot = Slot<Ret(Connection const&, Args...)>;

   public:
    Signal_impl(Combiner combiner, Group_compare const& group_compare)
        : connections_{group_compare}, combiner_{std::move(combiner)}
    {}

    Signal_impl(Signal_impl const& other)
    {
        auto const lock = std::lock_guard{other.mtx_};
        connections_    = other.connections_;
        combiner_       = other.combiner_;
    }

    Signal_impl(Signal_impl&& other)
    {
        auto const lock = std::lock_guard{other.mtx_};
        connections_    = std::move(other.connections_);
        combiner_       = std::move(other.combiner_);
    }

    auto operator=(Signal_impl const& other) -> Signal_impl&
    {
        if (this != &other) {
            auto lhs_lock = std::unique_lock{this->mtx_, std::defer_lock};
            auto rhs_lock = std::unique_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            connections_ = other.connections_;
            combiner_    = other.combiner_;
        }
        return *this;
    }

    auto operator=(Signal_impl&& other) -> Signal_impl&
    {
        if (this != &other) {
            auto lhs_lock = std::unique_lock{this->mtx_, std::defer_lock};
            auto rhs_lock = std::unique_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            connections_ = std::move(other.connections_);
            combiner_    = std::move(other.combiner_);
        }
        return *this;
    }

   public:
    auto connect(Slot_type const& slot, Position position) -> Connection
    {
        auto c_impl     = std::make_shared<Connection_impl<Signature>>(slot);
        auto const lock = std::lock_guard{mtx_};
        position == Position::at_front ? connections_.front.push_front(c_impl)
                                       : connections_.back.push_back(c_impl);
        return Connection(c_impl);
    }

    auto connect(Group const& group, Slot_type const& slot, Position position)
        -> Connection
    {
        auto c_impl     = std::make_shared<Connection_impl<Signature>>(slot);
        auto const lock = std::lock_guard{mtx_};
        position == Position::at_front
            ? connections_.grouped[group].push_front(c_impl)
            : connections_.grouped[group].push_back(c_impl);
        return Connection(c_impl);
    }

    auto connect_extended(Extended_slot const& ext_slot, Position position)
        -> Connection
    {
        auto c_impl = std::make_shared<Connection_impl<Signature>>();
        auto c      = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        auto const lock = std::lock_guard{mtx_};
        position == Position::at_front ? connections_.front.push_front(c_impl)
                                       : connections_.back.push_back(c_impl);
        return c;
    }

    auto connect_extended(Group const& group,
                          Extended_slot const& ext_slot,
                          Position position) -> Connection
    {
        auto c_impl = std::make_shared<Connection_impl<Signature>>();
        auto c      = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        auto const lock = std::lock_guard{mtx_};
        position == Position::at_front
            ? connections_.grouped[group].push_front(c_impl)
            : connections_.grouped[group].push_back(c_impl);
        return c;
    }

    void disconnect(Group const& group)
    {
        auto const lock = std::lock_guard{mtx_};
        for (auto& connection : connections_.grouped[group]) {
            connection->disconnect();
        }
        connections_.grouped.erase(group);
    }

    void disconnect_all_slots()
    {
        auto const lock = std::lock_guard{mtx_};
        for (auto& connection : connections_.front) {
            connection->disconnect();
        }
        for (auto& group : connections_.grouped) {
            for (auto& connection : group.second) {
                connection->disconnect();
            }
        }
        for (auto& connection : connections_.back) {
            connection->disconnect();
        }
        connections_.front.clear();
        connections_.grouped.clear();
        connections_.back.clear();
    }

    auto empty() const -> bool
    {
        auto const lock = std::lock_guard{mtx_};
        for (auto& connection : connections_.front) {
            if (connection->connected())
                return false;
        }
        for (auto& group : connections_.grouped) {
            for (auto& connection : group.second) {
                if (connection->connected())
                    return false;
            }
        }
        for (auto& connection : connections_.back) {
            if (connection->connected())
                return false;
        }
        return true;
    }

    auto num_slots() const -> std::size_t
    {
        auto const lock = std::lock_guard{mtx_};
        auto size       = 0uL;
        for (auto& connection : connections_.front) {
            if (connection->connected())
                ++size;
        }
        for (auto& group : connections_.grouped) {
            for (auto& connection : group.second) {
                if (connection->connected())
                    ++size;
            }
        }
        for (auto& connection : connections_.back) {
            if (connection->connected())
                ++size;
        }
        return size;
    }

    template <typename... Arguments>
    auto operator()(Arguments&&... args) -> Result_type
    {
        if (!this->enabled())
            return Result_type();
        auto slots = bind_args(std::forward<Arguments>(args)...);
        auto lock  = std::unique_lock{mtx_};
        auto comb  = combiner_;
        lock.unlock();
        return comb(Bound_slot_iterator{std::begin(slots)},
                    Bound_slot_iterator{std::end(slots)});
    }

    template <typename... Arguments>
    auto operator()(Arguments&&... args) const -> Result_type
    {
        if (!this->enabled())
            return Result_type();
        auto slots            = bind_args(std::forward<Arguments>(args)...);
        auto lock             = std::unique_lock{mtx_};
        auto const const_comb = combiner_;
        lock.unlock();
        return const_comb(Bound_slot_iterator{std::begin(slots)},
                          Bound_slot_iterator{std::end(slots)});
    }

    auto combiner() const -> Combiner
    {
        auto const lock = std::lock_guard{mtx_};
        return combiner_;
    }

    void set_combiner(Combiner const& comb)
    {
        auto const lock = std::lock_guard{mtx_};
        combiner_       = comb;
    }

    auto enabled() const -> bool
    {
        auto const lock = std::lock_guard{mtx_};
        return enabled_;
    }

    void enable()
    {
        auto const lock = std::lock_guard{mtx_};
        enabled_        = true;
    }

    void disable()
    {
        auto const lock = std::lock_guard{mtx_};
        enabled_        = false;
    }

   private:
    using Bound_slot_container = std::vector<std::function<Ret()>>;
    using Bound_slot_iterator =
        Slot_iterator<typename Bound_slot_container::iterator>;

    class Connection_container {
       public:
        using Position_container =
            std::deque<std::shared_ptr<Connection_impl<Signature>>>;
        using Group_container =
            std::map<Group, Position_container, Group_compare>;

       public:
        Position_container front;
        Group_container grouped;
        Position_container back;

       public:
        Connection_container() = default;
        Connection_container(Group_compare const& compare) : grouped{compare} {}
    };

   private:
    bool enabled_ = true;
    Connection_container connections_;
    Combiner combiner_;
    mutable Mutex mtx_;

   private:
    // Binds parameters to each Slot so Combiner does not need to know them.
    template <typename... Params>
    auto bind_args(Params&&... args) const -> Bound_slot_container
    {
        auto bound_slots = Bound_slot_container{};
        // Helper Function
        auto bind_slots = [&bound_slots, &args...](auto& conn_container) {
            for (auto& connection : conn_container) {
                if (connection->connected() && !connection->blocked() &&
                    !connection->get_slot().expired()) {
                    auto& slot = connection->get_slot();
                    bound_slots.push_back(
                        [slot, &args...] { return slot(args...); });
                }
            }
        };
        // Bind arguments to all three types of connected Slots.
        auto const lock = std::lock_guard{mtx_};
        bind_slots(connections_.front);
        for (auto& group : connections_.grouped)
            bind_slots(group.second);
        bind_slots(connections_.back);
        return bound_slots;
    }
};

}  // namespace sig

#endif  // SIGNALS_DETAIL_SIGNAL_IMPL_HPP

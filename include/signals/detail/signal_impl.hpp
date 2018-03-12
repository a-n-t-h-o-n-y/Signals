#ifndef DETAIL_SIGNAL_IMPL_HPP
#define DETAIL_SIGNAL_IMPL_HPP
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
    using Result_type = typename Combiner::Result_type;
    using Signature = Ret(Args...);
    using Slot_type = Slot<Signature, Slot_function>;
    using Extended_slot = Slot<Ret(const Connection&, Args...)>;

    Signal_impl(Combiner combiner, const Group_compare& group_compare)
        : connections_{group_compare}, combiner_{std::move(combiner)} {}

    Signal_impl(const Signal_impl& other) {
        std::lock_guard<Mutex> lock{other.mtx_};
        connections_ = other.connections_;
        combiner_ = other.combiner_;
    }

    Signal_impl(Signal_impl&& other) {
        std::lock_guard<Mutex> lock{other.mtx_};
        connections_ = std::move(other.connections_);
        combiner_ = std::move(other.combiner_);
    }

    Signal_impl& operator=(const Signal_impl& other) {
        if (this != &other) {
            std::unique_lock<Mutex> lhs_lck{this->mtx_, std::defer_lock};
            std::unique_lock<Mutex> rhs_lck{other.mtx_, std::defer_lock};
            std::lock(lhs_lck, rhs_lck);
            connections_ = other.connections_;
            combiner_ = other.combiner_;
        }
        return *this;
    }

    Signal_impl& operator=(Signal_impl&& other) {
        if (this != &other) {
            std::unique_lock<Mutex> lhs_lck{this->mtx_, std::defer_lock};
            std::unique_lock<Mutex> rhs_lck{other.mtx_, std::defer_lock};
            std::lock(lhs_lck, rhs_lck);
            connections_ = std::move(other.connections_);
            combiner_ = std::move(other.combiner_);
        }
        return *this;
    }

    Connection connect(const Slot_type& slot, Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature>>(slot);
        std::lock_guard<Mutex> lock(mtx_);
        position == Position::at_front ? connections_.front.push_front(c_impl)
                                       : connections_.back.push_back(c_impl);
        return Connection(c_impl);
    }

    Connection connect(const Group& group,
                       const Slot_type& slot,
                       Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature>>(slot);
        std::lock_guard<Mutex> lock(mtx_);
        position == Position::at_front
            ? connections_.grouped[group].push_front(c_impl)
            : connections_.grouped[group].push_back(c_impl);
        return Connection(c_impl);
    }

    Connection connect_extended(const Extended_slot& ext_slot,
                                Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature>>();
        auto c = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        std::lock_guard<Mutex> lock(mtx_);
        position == Position::at_front ? connections_.front.push_front(c_impl)
                                       : connections_.back.push_back(c_impl);
        return c;
    }

    Connection connect_extended(const Group& group,
                                const Extended_slot& ext_slot,
                                Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature>>();
        auto c = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        std::lock_guard<Mutex> lock(mtx_);
        position == Position::at_front
            ? connections_.grouped[group].push_front(c_impl)
            : connections_.grouped[group].push_back(c_impl);
        return c;
    }

    void disconnect(const Group& group) {
        std::lock_guard<Mutex> lock(mtx_);
        for (auto& connection : connections_.grouped[group]) {
            connection->disconnect();
        }
        connections_.grouped.erase(group);
    }

    void disconnect_all_slots() {
        std::lock_guard<Mutex> lock(mtx_);
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

    bool empty() const {
        std::lock_guard<Mutex> lock{mtx_};
        for (auto& connection : connections_.front) {
            if (connection->connected()) {
                return false;
            }
        }
        for (auto& group : connections_.grouped) {
            for (auto& connection : group.second) {
                if (connection->connected()) {
                    return false;
                }
            }
        }
        for (auto& connection : connections_.back) {
            if (connection->connected()) {
                return false;
            }
        }
        return true;
    }

    std::size_t num_slots() const {
        std::lock_guard<Mutex> lock{mtx_};
        std::size_t size{0};
        for (auto& connection : connections_.front) {
            if (connection->connected()) {
                ++size;
            }
        }
        for (auto& group : connections_.grouped) {
            for (auto& connection : group.second) {
                if (connection->connected()) {
                    ++size;
                }
            }
        }
        for (auto& connection : connections_.back) {
            if (connection->connected()) {
                ++size;
            }
        }
        return size;
    }

    template <typename... Arguments>
    Result_type operator()(Arguments&&... args) {
        if (!this->enabled()) {
            return Result_type();
        }
        auto slots = bind_args(std::forward<Arguments>(args)...);
        std::unique_lock<Mutex> lock{mtx_};
        auto comb = combiner_;
        lock.unlock();
        return comb(Bound_slot_iterator{std::begin(slots)},
                    Bound_slot_iterator{std::end(slots)});
    }

    template <typename... Arguments>
    Result_type operator()(Arguments&&... args) const {
        if (!this->enabled()) {
            return Result_type();
        }
        auto slots = bind_args(std::forward<Arguments>(args)...);
        std::unique_lock<Mutex> lock{mtx_};
        const Combiner const_comb = combiner_;
        lock.unlock();
        return const_comb(Bound_slot_iterator{std::begin(slots)},
                          Bound_slot_iterator{std::end(slots)});
    }

    Combiner combiner() const {
        std::lock_guard<Mutex> lock{mtx_};
        return combiner_;
    }

    void set_combiner(const Combiner& comb) {
        std::unique_lock<Mutex> lock{mtx_};
        combiner_ = comb;
    }

    bool enabled() const {
        std::lock_guard<Mutex> lock{mtx_};
        return enabled_;
    }

    void enable() {
        std::lock_guard<Mutex> lock{mtx_};
        enabled_ = true;
    }

    void disable() {
        std::lock_guard<Mutex> lock{mtx_};
        enabled_ = false;
    }

   private:
    using Bound_slot_container = std::vector<std::function<Ret()>>;
    using Bound_slot_iterator =
        Slot_iterator<typename Bound_slot_container::iterator>;

    struct Connection_container {
        using Position_container =
            std::deque<std::shared_ptr<Connection_impl<Signature>>>;
        using Group_container =
            std::map<Group, Position_container, Group_compare>;

        Connection_container() = default;
        Connection_container(const Group_compare& compare) : grouped{compare} {}
        Position_container front;
        Group_container grouped;
        Position_container back;
    };

    // Binds parameters to each Slot so Combiner does not need to know them.
    template <typename... Params>
    Bound_slot_container bind_args(Params&&... args) const {
        Bound_slot_container bound_slots;
        // Helper Function
        auto bind_slots = [&bound_slots, &args...](auto& conn_container) {
            for (auto& connection : conn_container) {
                if (connection->connected() && !connection->blocked() &&
                    !connection->get_slot().expired()) {
                    auto& slot = connection->get_slot();
                    bound_slots.push_back([slot, &args...] {
                        return slot(std::forward<Params>(args)...);
                    });
                }
            }
        };
        std::lock_guard<Mutex> lock{mtx_};
        // Bind arguments to all three types of connected Slots.
        bind_slots(connections_.front);
        for (auto& group : connections_.grouped) {
            bind_slots(group.second);
        }
        bind_slots(connections_.back);
        return bound_slots;
    }

    bool enabled_{true};
    Connection_container connections_;

    Combiner combiner_;
    mutable Mutex mtx_;
};

}  // namespace sig

#endif  // DETAIL_SIGNAL_IMPL_HPP

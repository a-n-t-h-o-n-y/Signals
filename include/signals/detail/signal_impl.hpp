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
#include <shared_mutex>
#include <utility>

namespace sig {

// Forward Declaration
template <typename Signature>
class Connection_impl;

// Forward declaration, so that Signature can be split into Ret and Args...
template <typename Signature,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename SharedMutex>
class Signal_impl;

template <typename Ret,
          typename... Args,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename SharedMutex>
class Signal_impl<Ret(Args...),
                  Combiner,
                  Group,
                  GroupCompare,
                  SlotFunction,
                  SharedMutex> {
   public:
    // Types
    using Result_t = typename Combiner::Result_t;
    using Signature_t = Ret(Args...);
    using Group_t = Group;
    using Group_compare_t = GroupCompare;
    using Combiner_t = Combiner;
    using Slot_function_t = SlotFunction;
    using Slot_t = Slot<Signature_t, Slot_function_t>;
    using Extended_slot_t = Slot<Ret(const Connection&, Args...)>;

    Signal_impl(Combiner_t combiner, const Group_compare_t& group_compare)
        : connections_{group_compare}, combiner_{std::move(combiner)} {}

    Signal_impl(const Signal_impl& other) {
        std::shared_lock<SharedMutex> lock{other.mtx_};
        connections_ = other.connections_;
        combiner_ = other.combiner_;
    }

    Signal_impl(Signal_impl&& other) {
        std::lock_guard<SharedMutex> lock{other.mtx_};
        connections_ = std::move(other.connections_);
        combiner_ = std::move(other.combiner_);
    }

    Signal_impl& operator=(const Signal_impl& other) {
        if (this != &other) {
            std::unique_lock<SharedMutex> lhs_lock{this->mtx_, std::defer_lock};
            std::shared_lock<SharedMutex> rhs_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            connections_ = other.connections_;
            combiner_ = other.combiner_;
        }
        return *this;
    }

    Signal_impl& operator=(Signal_impl&& other) {
        if (this != &other) {
            std::unique_lock<SharedMutex> lhs_lock{this->mtx_, std::defer_lock};
            std::unique_lock<SharedMutex> rhs_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            connections_ = std::move(other.connections_);
            combiner_ = std::move(other.combiner_);
        }
        return *this;
    }

    Connection connect(const Slot_t& slot, Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>(slot);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front ? connections_.front.push_front(c_impl)
                                       : connections_.back.push_back(c_impl);
        return Connection(c_impl);
    }

    Connection connect(const Group_t& group,
                       const Slot_t& slot,
                       Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>(slot);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front
            ? connections_.grouped[group].push_front(c_impl)
            : connections_.grouped[group].push_back(c_impl);
        return Connection(c_impl);
    }

    Connection connect_extended(const Extended_slot_t& ext_slot,
                                Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>();
        auto c = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front ? connections_.front.push_front(c_impl)
                                       : connections_.back.push_back(c_impl);
        return c;
    }

    Connection connect_extended(const Group_t& group,
                                const Extended_slot_t& ext_slot,
                                Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>();
        auto c = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front
            ? connections_.grouped[group].push_front(c_impl)
            : connections_.grouped[group].push_back(c_impl);
        return c;
    }

    void disconnect(const Group_t& group) {
        std::lock_guard<SharedMutex> lock(mtx_);
        for (auto& connection : connections_.grouped[group]) {
            connection->disconnect();
        }
        connections_.grouped.erase(group);
    }

    void disconnect_all_slots() {
        std::lock_guard<SharedMutex> lock(mtx_);
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
        std::shared_lock<SharedMutex> lock{mtx_};
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
        std::shared_lock<SharedMutex> lock{mtx_};
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
    Result_t operator()(Arguments&&... args) {
        auto slots = bind_args(std::forward<Arguments>(args)...);
        std::shared_lock<SharedMutex> lock{mtx_};
        auto comb = combiner_;
        lock.unlock();
        return comb(Bound_slot_iterator{std::begin(slots)},
                    Bound_slot_iterator{std::end(slots)});
    }

    template <typename... Arguments>
    Result_t operator()(Arguments&&... args) const {
        auto slots = bind_args(std::forward<Arguments>(args)...);
        std::shared_lock<SharedMutex> lock{mtx_};
        const Combiner_t const_comb = combiner_;
        lock.unlock();
        return const_comb(Bound_slot_iterator{std::begin(slots)},
                          Bound_slot_iterator{std::end(slots)});
    }

    Combiner_t combiner() const {
        std::shared_lock<SharedMutex> lock{mtx_};
        return combiner_;
    }

    void set_combiner(const Combiner_t& comb) {
        std::unique_lock<SharedMutex> lock{mtx_};
        combiner_ = comb;
    }

   private:
    using Bound_slot_container = std::vector<std::function<Ret()>>;
    using Bound_slot_iterator =
        Slot_iterator<typename Bound_slot_container::iterator>;

    struct Connection_container {
        using Position_container =
            std::deque<std::shared_ptr<Connection_impl<Signature_t>>>;
        using Group_container =
            std::map<Group_t, Position_container, Group_compare_t>;

        Connection_container() = default;
        Connection_container(const GroupCompare& compare) : grouped{compare} {}
        Position_container front;
        Group_container grouped;
        Position_container back;
    };

    // Prepares the functions to be processed by the Combiner.
    // Returns a container of std::functions with signature Ret().
    template <typename... Arguments>
    Bound_slot_container bind_args(Arguments&&... args) const {
        Bound_slot_container bound_slots;
        auto bind_slots = [&bound_slots, &args...](auto& conn_container) {
            for (auto& connection : conn_container) {
                if (connection->connected() && !connection->blocked() &&
                    !connection->get_slot().expired()) {
                    auto& slot = connection->get_slot();
                    bound_slots.push_back([slot, &args...] {
                        return slot(std::forward<Arguments>(args)...);
                    });
                }
            }
        };
        std::shared_lock<SharedMutex> lock{mtx_};
        bind_slots(connections_.front);
        for (auto& group : connections_.grouped) {
            bind_slots(group.second);
        }
        bind_slots(connections_.back);
        return bound_slots;
    }

    Connection_container connections_;

    Combiner_t combiner_;
    mutable SharedMutex mtx_;
};

}  // namespace sig

#endif  // DETAIL_SIGNAL_IMPL_HPP

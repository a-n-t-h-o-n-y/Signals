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
          typename SharedMutex = std::shared_timed_mutex>
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
        : grouped_connections_{group_compare}, combiner_{std::move(combiner)} {}

    Signal_impl(const Signal_impl& other) {
        std::shared_lock<SharedMutex> lock{other.mtx_};
        front_connections_ = other.front_connections_;
        grouped_connections_ = other.grouped_connections_;
        back_connections_ = other.back_connections_;
        combiner_ = other.combiner_;
    }

    Signal_impl(Signal_impl&& other) {
        std::lock_guard<SharedMutex> lock{other.mtx_};
        front_connections_ = std::move(other.front_connections_);
        grouped_connections_ = std::move(other.grouped_connections_);
        back_connections_ = std::move(other.back_connections_);
        combiner_ = std::move(other.combiner_);
    }

    Signal_impl& operator=(const Signal_impl& other) {
        if (this != &other) {
            std::unique_lock<SharedMutex> lhs_lock{this->mtx_, std::defer_lock};
            std::shared_lock<SharedMutex> rhs_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            front_connections_ = other.front_connections_;
            grouped_connections_ = other.grouped_connections_;
            back_connections_ = other.back_connections_;
            combiner_ = other.combiner_;
        }
        return *this;
    }

    Signal_impl& operator=(Signal_impl&& other) {
        if (this != &other) {
            std::unique_lock<SharedMutex> lhs_lock{this->mtx_, std::defer_lock};
            std::unique_lock<SharedMutex> rhs_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            front_connections_ = std::move(other.front_connections_);
            grouped_connections_ = std::move(other.grouped_connections_);
            back_connections_ = std::move(other.back_connections_);
            combiner_ = std::move(other.combiner_);
        }
        return *this;
    }

    Connection connect(const Slot_t& slot, Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>(slot);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front ? front_connections_.push_front(c_impl)
                                       : back_connections_.push_back(c_impl);
        return Connection(c_impl);
    }

    Connection connect(const Group_t& group,
                       const Slot_t& slot,
                       Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>(slot);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front
            ? grouped_connections_[group].push_front(c_impl)
            : grouped_connections_[group].push_back(c_impl);
        return Connection(c_impl);
    }

    Connection connect_extended(const Extended_slot_t& ext_slot,
                                Position position) {
        auto c_impl = std::make_shared<Connection_impl<Signature_t>>();
        auto c = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        std::lock_guard<SharedMutex> lock(mtx_);
        position == Position::at_front ? front_connections_.push_front(c_impl)
                                       : back_connections_.push_back(c_impl);
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
            ? grouped_connections_[group].push_front(c_impl)
            : grouped_connections_[group].push_back(c_impl);
        return c;
    }

    void disconnect(const Group_t& group) {
        std::lock_guard<SharedMutex> lock(mtx_);
        for (auto& c_impl_ptr : grouped_connections_[group]) {
            c_impl_ptr->disconnect();
        }
        grouped_connections_.erase(group);
    }

    void disconnect_all_slots() {
        std::lock_guard<SharedMutex> lock(mtx_);
        for (auto& c_impl_ptr : front_connections_) {
            c_impl_ptr->disconnect();
        }
        for (auto& gc_pair : grouped_connections_) {
            for (auto& c_impl_ptr : gc_pair.second) {
                c_impl_ptr->disconnect();
            }
        }
        for (auto& c_impl_ptr : back_connections_) {
            c_impl_ptr->disconnect();
        }
        front_connections_.clear();
        grouped_connections_.clear();
        back_connections_.clear();
    }

    bool empty() const {
        std::shared_lock<SharedMutex> lock{mtx_};
        for (auto& c_impl_ptr : front_connections_) {
            if (c_impl_ptr->connected()) {
                return false;
            }
        }
        for (auto& gc_pair : grouped_connections_) {
            for (auto& c_impl_ptr : gc_pair.second) {
                if (c_impl_ptr->connected()) {
                    return false;
                }
            }
        }
        for (auto& c_impl_ptr : back_connections_) {
            if (c_impl_ptr->connected()) {
                return false;
            }
        }
        return true;
    }

    std::size_t num_slots() const {
        std::shared_lock<SharedMutex> lock{mtx_};
        std::size_t size{0};
        for (auto& c_impl_ptr : front_connections_) {
            if (c_impl_ptr->connected()) {
                ++size;
            }
        }
        for (auto& gc_pair : grouped_connections_) {
            for (auto& c_impl_ptr : gc_pair.second) {
                if (c_impl_ptr->connected()) {
                    ++size;
                }
            }
        }
        for (auto& c_impl_ptr : back_connections_) {
            if (c_impl_ptr->connected()) {
                ++size;
            }
        }
        return size;
    }

    template <typename... Arguments>
    Result_t operator()(Arguments&&... args) {
        auto slots = bind_args(std::forward<Arguments>(args)...);
        return combiner_(
            Slot_iterator<typename Container_t::iterator>(std::begin(slots)),
            Slot_iterator<typename Container_t::iterator>(std::end(slots)));
    }

    template <typename... Arguments>
    Result_t operator()(Arguments&&... args) const {
        auto slots = bind_args(std::forward<Arguments>(args)...);
        std::shared_lock<SharedMutex> lock{mtx_};
        const Combiner_t const_comb = combiner_;
        lock.unlock();
        return const_comb(
            Slot_iterator<typename Container_t::iterator>(std::begin(slots)),
            Slot_iterator<typename Container_t::iterator>(std::end(slots)));
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
    using Container_t = std::vector<std::function<Ret()>>;
    using Position_container_t =
        std::deque<std::shared_ptr<Connection_impl<Signature_t>>>;
    using Group_container_t =
        std::map<Group_t, Position_container_t, Group_compare_t>;

    // Prepares the functions to be processed by the combiner.
    // Returns a container of std::functions with signature Ret().
    template <typename... Arguments>
    Container_t bind_args(Arguments&&... args) const {
        Container_t bound_slot_container;
        std::shared_lock<SharedMutex> lock{mtx_};
        for (auto& c_impl_ptr : front_connections_) {
            if (c_impl_ptr->connected() && !c_impl_ptr->blocked() &&
                !c_impl_ptr->get_slot().expired()) {
                bound_slot_container.push_back([&c_impl_ptr, &args...] {
                    return c_impl_ptr->get_slot()(
                        std::forward<Arguments>(args)...);
                });
            }
        }
        for (auto& gc_pair : grouped_connections_) {
            for (auto& c_impl_ptr : gc_pair.second) {
                if (c_impl_ptr->connected() && !c_impl_ptr->blocked() &&
                    !c_impl_ptr->get_slot().expired()) {
                    bound_slot_container.push_back([&c_impl_ptr, &args...] {
                        return c_impl_ptr->get_slot()(
                            std::forward<Arguments>(args)...);
                    });
                }
            }
        }
        for (auto& c_impl_ptr : back_connections_) {
            if (c_impl_ptr->connected() && !c_impl_ptr->blocked() &&
                !c_impl_ptr->get_slot().expired()) {
                bound_slot_container.push_back([&c_impl_ptr, &args...] {
                    return c_impl_ptr->get_slot()(
                        std::forward<Arguments>(args)...);
                });
            }
        }
        return bound_slot_container;
    }

    // Connections are stored here
    Position_container_t front_connections_;
    Group_container_t grouped_connections_;
    Position_container_t back_connections_;

    Combiner_t combiner_;
    mutable SharedMutex mtx_;
};

}  // namespace sig

#endif  // DETAIL_SIGNAL_IMPL_HPP

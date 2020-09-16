#ifndef SIGNALS_SIGNAL_HPP
#define SIGNALS_SIGNAL_HPP
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <tuple>
#include <utility>
#include <vector>

#include "connection.hpp"
#include "detail/connection_impl.hpp"
#include "detail/slot_iterator.hpp"
#include "position.hpp"
#include "signal_fwd.hpp"
#include "slot_fwd.hpp"

namespace sig {

/// Represents a signal that can be sent out to notify connected Slots.
/** Slots(functions) can be connected(registered) to a Signal. When the Signal
 *  is called(with operator()), the arguments passed to the Signal will be
 *  forwarded to each Slot. The return value of the Signal call is an Optional
 *  that defaults to the return value of the last Slot called by the Signal.
 *  \param Ret The return type of the Slot function
 *  \param Args The argument types to the Slot function
 *  \param Combiner that will return manage returning the Slots' return values.
 *  \param Group Type used to group Slots together to determine call order.
 *  \param Group_compare Comparison functor to determine order to call Slots.
 *  \param Slot_function Function wrapper type for Slots
 *  \param Mutex Mutex type for multithreaded use of Signals
 *  \sa Slot signal_fwd.hpp */
template <typename Ret,
          typename... Args,
          typename Combiner,
          typename Group,
          typename Group_compare,
          typename Slot_function,
          typename Mutex>
class Signal<Ret(Args...),
             Combiner,
             Group,
             Group_compare,
             Slot_function,
             Mutex> {
   private:
    using Lock_t = std::scoped_lock<Mutex>;

   public:
    using Result_type = typename Combiner::Result_type;
    using Signature   = Ret(Args...);
    using Slot_type   = Slot<Signature, Slot_function>;
    using Extended_slot_function =
        std::function<Ret(Connection const&, Args...)>;
    using Extended_slot =
        Slot<Ret(Connection const&, Args...), Extended_slot_function>;
    using Arguments = std::tuple<Args...>;

    /// Number of arguments the Signal takes.
    static constexpr int arity = std::tuple_size_v<Arguments>;

    /// Access to the type of argument number \p N.
    template <std::size_t N>
    using arg = typename std::tuple_element_t<N, Arguments>;

   public:
    /// Constructs an empty Signal with a combiner and a group_compare.
    /** \param combiner Slot return value combiner object.
     *  \param group_compare Comparison functor for Slot call order. */
    explicit Signal(Combiner const& combiner           = Combiner(),
                    Group_compare const& group_compare = Group_compare())
        : connections_{group_compare}, combiner_{std::move(combiner)}
    {}

    Signal(Signal const& other)
    {
        auto const lock = Lock_t{other.mtx_};
        connections_    = other.connections_;
        combiner_       = other.combiner_;
    }

    Signal(Signal&& other) noexcept
    {
        auto const lock = Lock_t{other.mtx_};
        connections_    = std::move(other.connections_);
        combiner_       = std::move(other.combiner_);
        tracker_        = std::move(other.tracker_);
    }

    auto operator=(Signal const& other) -> Signal&
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

    auto operator=(Signal&& other) -> Signal&
    {
        if (this != &other) {
            auto lhs_lock = std::unique_lock{this->mtx_, std::defer_lock};
            auto rhs_lock = std::unique_lock{other.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            connections_ = std::move(other.connections_);
            combiner_    = std::move(other.combiner_);
            tracker_     = std::move(other.tracker_);
        }
        return *this;
    }

    ~Signal() = default;

   public:
    /// Connect a Slot to *this either at the front or back of call queue.
    /** The Slot is inserted into a queue either at the front or the back
     *  depending on \p position.
     *  \param slot The Slot to connection to *this
     *  \param position The call position of \p slot
     *  \returns A Connection object referring to the Signal/Slot Connection.
     *  \sa Position Slot */
    auto connect(Slot_type const& slot, Position position = Position::at_back)
        -> Connection
    {
        auto c_impl     = std::make_shared<Connection_impl<Signature>>(slot);
        auto const lock = Lock_t{mtx_};
        if (position == Position::at_front)
            connections_.front.emplace(std::begin(connections_.front), c_impl);
        else
            connections_.back.push_back(c_impl);
        return Connection(c_impl);
    }

    /// Connect a Slot to *this in a particular call group.
    /** Slots are inserted into groups, and inserted into the group by the \p
     *  position parameter. Signals are called the order: at_front without
     *  group number, groups by the Group_compare function(each group from
     *  at_front to at_back), and finally by at_back without group.
     *  \param group The group the Slot will be a member of.
     *  \param slot The Slot to be connected.
     *  \param position The position in the group that the Slot is added to.
     *  \returns A Connection object referring to the Signal/Slot Connection. */
    auto connect(Group const& group,
                 Slot_type const& slot,
                 Position position = Position::at_back) -> Connection
    {
        auto c_impl     = std::make_shared<Connection_impl<Signature>>(slot);
        auto const lock = Lock_t{mtx_};
        if (position == Position::at_front) {
            auto& group_container = connections_.grouped[group];
            group_container.emplace(std::begin(group_container), c_impl);
        }
        else
            connections_.grouped[group].push_back(c_impl);
        return Connection(c_impl);
    }

    /// Connect an extended Slot to *this by \p position.
    /** An extended Slot is a Slot that has the signature of the Signal, but
     *  with an extra Connection parameter as the first parameter. This is
     *  useful if the Slot needs to access its own Connection(possibly to
     *  disconnect itself). When calling the Signal, the Connection of the
     *  extended Slot is automatically bound to its first parameter.
     *  \param ext_slot The connected Slot, has signature:
     *  Ret ext_slot(Connection const&, Args...)
     *  \param position The call position of \p ext_slot.
     *  \returns A Connection object referring to the Signal/Slot Connection. */
    auto connect_extended(Extended_slot const& ext_slot,
                          Position position = Position::at_back) -> Connection
    {
        auto c_impl = std::make_shared<Connection_impl<Signature>>();
        auto c      = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        auto const lock = Lock_t{mtx_};
        if (position == Position::at_front)
            connections_.front.emplace(std::begin(connections_.front), c_impl);
        else
            connections_.back.push_back(c_impl);
        return c;
    }

    /// Connect an extended Slot to *this in a particular call group.
    /** Slots are inserted into groups, and inserted into the group by the \p
     *  position parameter. Signals are called in the order: at_front without
     *  group number, groups by the Group_compare function(each group from
     *  at_front to at_back), and finally by at_back without group.
     *  An extended Slot is a Slot that has the signature of the Signal, but
     *  with an extra Connection parameter as the first parameter. This is
     *  useful if the Slot needs to access its own Connection(possibly to
     *  disconnect itself). When calling the Signal, the Connection of the
     *  extended Slot is automatically bound to its first parameter.
     *  \param group The group the Slot will be a member of.
     *  \param ext_slot The connected Slot, has signature:
     *  Ret ext_slot(Connection const&, Args...)
     *  \param position The position in the group that the Slot is added to.
     *  \returns A Connection object referring to the Signal/Slot Connection. */
    auto connect_extended(Group const& group,
                          Extended_slot const& ext_slot,
                          Position position = Position::at_back) -> Connection
    {
        auto c_impl = std::make_shared<Connection_impl<Signature>>();
        auto c      = Connection(c_impl);
        c_impl->emplace_extended(ext_slot, c);
        auto const lock = Lock_t{mtx_};
        if (position == Position::at_front) {
            auto& group_container = connections_.grouped[group];
            group_container.emplace(std::begin(group_container), c_impl);
        }
        else
            connections_.grouped[group].push_back(c_impl);
        return c;
    }

    /// Disconnect all Slots in a given group.
    /** \param group The group to disconnect. */
    void disconnect(Group const& group)
    {
        auto const lock = Lock_t{mtx_};
        for (auto& connection : connections_.grouped[group]) {
            connection->disconnect();
        }
        connections_.grouped.erase(group);
    }

    /// Disconnect all Slots attached to *this.
    void disconnect_all_slots()
    {
        auto const lock = Lock_t{mtx_};
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

    /// Query whether or not *this has any Slots connected to it.
    /** \returns True if *this has no Slots attached, false otherwise. */
    auto empty() const -> bool
    {
        auto const lock = Lock_t{mtx_};
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

    /// Access the number of Slots connected to *this.
    /** \returns The number of Slots currently connected to *this. */
    auto num_slots() const -> std::size_t
    {
        auto const lock = Lock_t{mtx_};
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

    /// Call operator to call all connected Slots.
    /** All arguments to this call operator are passed onto the Slots. The Slots
     *  are called by how they were attached to *this. By default this returns
     *  the return value of the last Slot that was called.
     *  \param args The arguments you are passing onto the Slots.
     *  \returns An Optional containing a value determined by the Combiner. */
    template <typename... Params>
    auto operator()(Params&&... args) -> Result_type
    {
        if (!this->enabled())
            return Result_type();
        auto slots = bind_args(std::forward<Params>(args)...);
        auto lock  = std::unique_lock{mtx_};
        auto comb  = combiner_;
        lock.unlock();
        return comb(Bound_slot_iterator{std::begin(slots)},
                    Bound_slot_iterator{std::end(slots)});
    }

    /// Call operator to call all connected Slots.
    /** All arguments to this call operator are passed onto the Slots. The Slots
     *  are called by how they were attached to *this. By default this returns
     *  the return value of the last Slot that was called. const overload is
     *  called with a const Combiner.
     *  \param args The arguments you are passing onto the Slots.
     *  \returns An Optional containing a value determined by the Combiner. */
    template <typename... Params>
    auto operator()(Params&&... args) const -> Result_type
    {
        if (!this->enabled())
            return Result_type();
        auto slots            = bind_args(std::forward<Params>(args)...);
        auto lock             = std::unique_lock{mtx_};
        auto const const_comb = combiner_;
        lock.unlock();
        return const_comb(Bound_slot_iterator{std::begin(slots)},
                          Bound_slot_iterator{std::end(slots)});
    }

    /// Access to the Combiner object.
    /** \returns A copy of the Combiner object used by *this. */
    auto combiner() const -> Combiner
    {
        auto const lock = Lock_t{mtx_};
        return combiner_;
    }

    /// Set the Combiner object to a new value.
    /** A Combiner is a functor that takes a range of input iterators, it
     *  dereferences each iterator in the range and returns some value as a
     *  Result_type.
     *  \params comb The Combiner object to set for *this. */
    void set_combiner(Combiner const& comb)
    {
        auto const lock = Lock_t{mtx_};
        combiner_       = comb;
    }

    /// Shared pointer that can track the lifetime of this Signal.
    auto get_tracker() const -> std::shared_ptr<int>
    {
        if (!tracker_.has_value())
            tracker_.emplace(std::make_shared<int>(0));
        return *tracker_;
    }

    /// Query whether or not the Signal is enabled.
    /** A disabled Signal does not call any connected Slots when the call
     *  operator is summoned.
     *  \returns True if *this is enabled, false otherwise. */
    auto enabled() const -> bool
    {
        auto const lock = Lock_t{mtx_};
        return enabled_;
    }

    /// Enable the Signal.
    /** Connected Slots will be called when call operator is summoned. */
    void enable()
    {
        auto const lock = Lock_t{mtx_};
        enabled_        = true;
    }

    /// Disable the Signal.
    /** Connected Slots will _not_ be called when call operator is summoned. */
    void disable()
    {
        auto const lock = Lock_t{mtx_};
        enabled_        = false;
    }

   private:
    using Bound_slot_container = std::vector<std::function<Ret()>>;
    using Bound_slot_iterator =
        Slot_iterator<typename Bound_slot_container::iterator>;

    class Connection_container {
       public:
        using Position_container =
            std::vector<std::shared_ptr<Connection_impl<Signature>>>;
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
    mutable std::optional<std::shared_ptr<int>> tracker_;
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
                    // you get lucky the args... are still in scope while needed
                    bound_slots.push_back(
                        [&slot, &args...] { return slot(args...); });
                }
            }
        };
        // Bind arguments to all three types of connected Slots.
        auto const lock = Lock_t{mtx_};
        bind_slots(connections_.front);
        for (auto& group : connections_.grouped)
            bind_slots(group.second);
        bind_slots(connections_.back);
        return bound_slots;
    }
};

}  // namespace sig
#endif  // SIGNAL_HPP

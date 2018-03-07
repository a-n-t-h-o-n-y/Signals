/// \file
/// \brief Contains the definiton for the Signal class.
#ifndef SIGNAL_HPP
#define SIGNAL_HPP
#include <signals/connection.hpp>
#include <signals/detail/signal_impl.hpp>
#include <signals/position.hpp>
#include <signals/signal_fwd.hpp>
#include <signals/slot_fwd.hpp>

#include <cstddef>
#include <functional>
#include <memory>
#include <tuple>
#include <utility>

namespace sig {

/// \brief Represents a signal that can be sent out to notify connected Slots.
///
/// Slots(functions) can be connected(registered) to a Signal. When the Signal
/// is called(with operator()), the arguments passed to the Signal will be
/// forwarded to each Slot. The return value of the Signal call is an Optional
/// that defaults to the return value of the last Slot called by the Signal.
/// \param Ret The return type of the Slot function
/// \param Args The argument types to the Slot function
/// \param Combiner Type that will return manage returning the Slots' return
/// values.
/// \param Group Type used to group Slots together to determine call order.
/// \param GroupCompare Comparison functor to determine order to call Slots.
/// \param SlotFunction Function wrapper type for Slots
/// \param Mutex Mutex type for multithreaded use of Signals
/// \sa Slot signal_fwd.hpp
template <typename Ret,
          typename... Args,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename Mutex>
class Signal<Ret(Args...), Combiner, Group, GroupCompare, SlotFunction, Mutex> {
   public:
    // Types
    using Signature_t = Ret(Args...);
    using Result_t = typename Combiner::Result_type;
    using Combiner_t = Combiner;
    using Group_t = Group;  // Don't need these, just use template params
    using Group_compare_t = GroupCompare;
    using Slot_function_t = SlotFunction;
    using Mutex_t = Mutex;
    using Slot_t = Slot<Signature_t, SlotFunction>;
    using Extended_slot_function_t =
        std::function<Ret(const Connection&, Args...)>;
    using Extended_slot_t =
        Slot<Ret(const Connection&, Args...), Extended_slot_function_t>;
    using Slot_result_t = typename SlotFunction::result_type;
    using Argument_t = std::tuple<Args...>;
    using Impl_t = Signal_impl<Signature_t,
                               Combiner_t,
                               Group_t,
                               Group_compare_t,
                               Slot_function_t,
                               Mutex_t>;

    /// Number of arguments the Signal takes.
    static const int arity = std::tuple_size<Argument_t>::value;

    /// \brief Access to each argument's type.
    ///
    /// \p type member is the type of argument \p n.
    template <unsigned n>
    class arg {
       public:
        using type = typename std::tuple_element<n, Argument_t>::type;
    };

    /// \brief Constructs an empty Signal with a combiner and a group_compare.
    ///
    /// \param combiner Slot return value combiner object.
    /// \param group_compare Comparison functor for Slot call order.
    explicit Signal(const Combiner_t& combiner = Combiner_t(),
                    const Group_compare_t& group_compare = Group_compare_t())
        : pimpl_{std::make_shared<Impl_t>(combiner, group_compare)} {}

    Signal(const Signal&) = delete;
    Signal(Signal&& x) noexcept = default;
    Signal& operator=(const Signal&) = delete;
    Signal& operator=(Signal&& x) noexcept = default;
    ~Signal() = default;

    /// \brief Connect a Slot to *this either at the front or back of call
    /// queue.
    ///
    /// The Slot is inserted into a queue either at the front or the back
    /// depending on \p position.
    /// \param slot The Slot to connection to *this
    /// \param position The call position of \p slot
    /// \returns A Connection object referring to the Signal/Slot Connection.
    /// \sa Position Slot
    Connection connect(const Slot_t& slot,
                       Position position = Position::at_back) {
        return pimpl_->connect(slot, position);
    }

    /// \brief Connect a Slot to *this in a particular call group.
    ///
    /// Slots are inserted into groups, and inserted into the group by the \p
    /// position parameter. Signals are called the order: at_front without
    /// group number, groups by the GroupCompare function(each group from
    /// at_front to at_back), and finally by at_back without group.
    /// \param group The group the Slot will be a member of.
    /// \param slot The Slot to be connected.
    /// \param position The position in the group that the Slot is added to.
    /// \returns A Connection object referring to the Signal/Slot Connection.
    Connection connect(const Group_t& group,
                       const Slot_t& slot,
                       Position position = Position::at_back) {
        return pimpl_->connect(group, slot, position);
    }

    /// \brief Connect an extended Slot to *this by \p position.
    ///
    /// An extended Slot is a Slot that has the signature of the Signal, but
    /// with an extra Connection parameter as the first parameter. This is
    /// useful if the Slot needs to access its own Connection(possibly to
    /// disconnect itself). When calling the Signal, the Connection of the
    /// extended Slot is automatically bound to its first parameter.
    /// \param ext_slot The connected Slot, has signature:
    /// Ret ext_slot(const Connection&, Args...)
    /// \param position The call position of \p ext_slot.
    /// \returns A Connection object referring to the Signal/Slot Connection.
    Connection connect_extended(const Extended_slot_t& ext_slot,
                                Position position = Position::at_back) {
        return pimpl_->connect_extended(ext_slot, position);
    }

    /// \brief Connect an extended Slot to *this in a particular call group.
    ///
    /// Slots are inserted into groups, and inserted into the group by the \p
    /// position parameter. Signals are called the order: at_front without
    /// group number, groups by the GroupCompare function(each group from
    /// at_front to at_back), and finally by at_back without group.
    /// An extended Slot is a Slot that has the signature of the Signal, but
    /// with an extra Connection parameter as the first parameter. This is
    /// useful if the Slot needs to access its own Connection(possibly to
    /// disconnect itself). When calling the Signal, the Connection of the
    /// extended Slot is automatically bound to its first parameter.
    /// \param group The group the Slot will be a member of.
    /// \param ext_slot The connected Slot, has signature:
    /// Ret ext_slot(const Connection&, Args...)
    /// \param position The position in the group that the Slot is added to.
    /// \returns A Connection object referring to the Signal/Slot Connection.
    Connection connect_extended(const Group_t& g,
                                const Extended_slot_t& es,
                                Position pos = Position::at_back) {
        return pimpl_->connect_extended(g, es, pos);
    }

    /// \brief Disconnect all Slots in a given group.
    ///
    /// \param group The group to disconnect.
    void disconnect(const Group_t& group) { pimpl_->disconnect(group); }

    /// Disconnect all Slots attached to *this.
    void disconnect_all_slots() { pimpl_->disconnect_all_slots(); }

    /// \brief Query whether or not *this has any Slots connected to it.
    ///
    /// \returns True if *this has no Slots attached, false otherwise.
    bool empty() const { return pimpl_->empty(); }

    /// \brief Access the number of Slots connected to *this.
    ///
    /// \returns The number of Slots currently connected to *this.
    std::size_t num_slots() const { return pimpl_->num_slots(); }

    /// \brief Call operator to call all connected Slots.
    ///
    /// All arguments to this call operator are passed onto the Slots. The Slots
    /// are called by how they were attached to *this. By default this returns
    /// the return value of the last Slot that was called.
    /// \param args The arguments you are passing onto the Slots.
    /// \returns An Optional containing a value determined by the Combiner.
    template <typename... Arguments>
    Result_t operator()(Arguments&&... args) {
        return enabled_ ? pimpl_->operator()(std::forward<Arguments>(args)...)
                        : Result_t();  // Empty Optional<T>
    }

    /// \brief Call operator to call all connected Slots.
    ///
    /// All arguments to this call operator are passed onto the Slots. The Slots
    /// are called by how they were attached to *this. By default this returns
    /// the return value of the last Slot that was called. const overload is
    /// called with a const Combiner.
    /// \param args The arguments you are passing onto the Slots.
    /// \returns An Optional containing a value determined by the Combiner.
    template <typename... Arguments>
    Result_t operator()(Arguments&&... args) const {
        // enabled check needs to be within mutex lock, put in Signal_impl.
        return enabled_ ? pimpl_->operator()(std::forward<Arguments>(args)...)
                        : Result_t();  // Empty Optional<T>
    }

    /// \brief Access to the Combiner object.
    ///
    /// \returns A copy of the Combiner object used by *this.
    Combiner_t combiner() const { return pimpl_->combiner(); }

    /// \brief Set the Combiner object to a new value.
    ///
    /// A Combiner is a functor that takes a range of input iterators, it
    /// dereferences each iterator in the range and returns some value as a
    /// Result_t.
    /// \params comb The Combiner object to set for *this.
    void set_combiner(const Combiner_t& comb) { pimpl_->set_combiner(comb); }

    /// \brief Ensures the Signal implementation will not disapear even if *this
    /// is destroyed.
    std::shared_ptr<void> lock_impl_as_void() const { return pimpl_; }

    /// \brief Ensures the Signal implementation will not disapear even if *this
    /// is destroyed. Also gives access to impl functions.
    std::shared_ptr<Impl_t> lock_impl() const { return pimpl_; }

    /// \brief Query whether or not the Signal is enabled. A disabled Signal
    /// does not call any connected Slots when the call operator is summoned.
    /// \returns True if *this is enabled, false otherwise.
    bool enabled() const { return enabled_; }

    /// \brief Enable the Signal.
    ///
    /// Connected Slots will be called when call operator is summoned. Safe when
    /// *this is already enabled.
    void enable() { enabled_ = true; }

    /// \brief Disable the Signal.
    ///
    /// Connected Slots will _not_ be called when call operator is summoned.
    /// Safe when *this is already disabled.
    void disable() { enabled_ = false; }

   private:
    std::shared_ptr<Impl_t> pimpl_;  // needed so that Slot can track a Signal.
    bool enabled_ = true;
};

template <typename Ret,
          typename... Args,
          typename Combiner,
          typename Group,
          typename GroupCompare,
          typename SlotFunction,
          typename Mutex>
const int
    Signal<Ret(Args...), Combiner, Group, GroupCompare, SlotFunction, Mutex>::
        arity;

}  // namespace sig
#endif  // SIGNAL_HPP

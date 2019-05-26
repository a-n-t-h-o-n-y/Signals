/// \file
/// \brief Contains the definition for the Signal class.
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
/// \param Group_compare Comparison functor to determine order to call Slots.
/// \param Slot_function Function wrapper type for Slots
/// \param Mutex Mutex type for multithreaded use of Signals
/// \sa Slot signal_fwd.hpp
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
   public:
    // Types
    using Result_type = typename Combiner::Result_type;
    using Signature = Ret(Args...);
    using Slot_type = Slot<Signature, Slot_function>;
    using Extended_slot_function =
        std::function<Ret(const Connection&, Args...)>;
    using Extended_slot =
        Slot<Ret(const Connection&, Args...), Extended_slot_function>;
    using Arguments = std::tuple<Args...>;
    using Implementation = Signal_impl<Signature,
                                       Combiner,
                                       Group,
                                       Group_compare,
                                       Slot_function,
                                       Mutex>;

    /// Number of arguments the Signal takes.
    static const int arity = std::tuple_size<Arguments>::value;

    /// \brief Access to each argument's type.
    ///
    /// \p type member is the type of argument \p n.
    template <unsigned n>
    struct Arg {
        using type = typename std::tuple_element<n, Arguments>::type;
    };

    /// \brief Constructs an empty Signal with a combiner and a group_compare.
    ///
    /// \param combiner Slot return value combiner object.
    /// \param group_compare Comparison functor for Slot call order.
    explicit Signal(const Combiner& combiner = Combiner(),
                    const Group_compare& group_compare = Group_compare())
        : pimpl_{std::make_shared<Implementation>(combiner, group_compare)} {}

    Signal(const Signal&) = delete;
    Signal(Signal&&) noexcept = default;
    Signal& operator=(const Signal&) = delete;
    Signal& operator=(Signal&&) noexcept = default;
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
    Connection connect(const Slot_type& slot,
                       Position position = Position::at_back) {
        return pimpl_->connect(slot, position);
    }

    /// \brief Connect a Slot to *this in a particular call group.
    ///
    /// Slots are inserted into groups, and inserted into the group by the \p
    /// position parameter. Signals are called the order: at_front without
    /// group number, groups by the Group_compare function(each group from
    /// at_front to at_back), and finally by at_back without group.
    /// \param group The group the Slot will be a member of.
    /// \param slot The Slot to be connected.
    /// \param position The position in the group that the Slot is added to.
    /// \returns A Connection object referring to the Signal/Slot Connection.
    Connection connect(const Group& group,
                       const Slot_type& slot,
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
    Connection connect_extended(const Extended_slot& ext_slot,
                                Position position = Position::at_back) {
        return pimpl_->connect_extended(ext_slot, position);
    }

    /// \brief Connect an extended Slot to *this in a particular call group.
    ///
    /// Slots are inserted into groups, and inserted into the group by the \p
    /// position parameter. Signals are called in the order: at_front without
    /// group number, groups by the Group_compare function(each group from
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
    Connection connect_extended(const Group& g,
                                const Extended_slot& es,
                                Position pos = Position::at_back) {
        return pimpl_->connect_extended(g, es, pos);
    }

    /// \brief Disconnect all Slots in a given group.
    ///
    /// \param group The group to disconnect.
    void disconnect(const Group& group) { pimpl_->disconnect(group); }

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
    template <typename... Params>
    Result_type operator()(Params&&... args) {
        return pimpl_->operator()(std::forward<Params>(args)...);
    }

    /// \brief Call operator to call all connected Slots.
    ///
    /// All arguments to this call operator are passed onto the Slots. The Slots
    /// are called by how they were attached to *this. By default this returns
    /// the return value of the last Slot that was called. const overload is
    /// called with a const Combiner.
    /// \param args The arguments you are passing onto the Slots.
    /// \returns An Optional containing a value determined by the Combiner.
    template <typename... Params>
    Result_type operator()(Params&&... args) const {
        return pimpl_->operator()(std::forward<Params>(args)...);
    }

    /// \brief Access to the Combiner object.
    ///
    /// \returns A copy of the Combiner object used by *this.
    Combiner combiner() const { return pimpl_->combiner(); }

    /// \brief Set the Combiner object to a new value.
    ///
    /// A Combiner is a functor that takes a range of input iterators, it
    /// dereferences each iterator in the range and returns some value as a
    /// Result_type.
    /// \params comb The Combiner object to set for *this.
    void set_combiner(const Combiner& comb) { pimpl_->set_combiner(comb); }

    /// \brief Ensures the Signal implementation will not disapear even if *this
    /// is destroyed.
    std::shared_ptr<void> lock_impl_as_void() const { return pimpl_; }

    /// \brief Ensures the Signal implementation will not disapear even if *this
    /// is destroyed. Also gives access to impl functions.
    std::shared_ptr<Implementation> lock_impl() const { return pimpl_; }

    /// \brief Query whether or not the Signal is enabled. A disabled Signal
    /// does not call any connected Slots when the call operator is summoned.
    /// \returns True if *this is enabled, false otherwise.
    bool enabled() const { return pimpl_->enabled(); }

    /// \brief Enable the Signal.
    ///
    /// Connected Slots will be called when call operator is summoned. Safe when
    /// *this is already enabled.
    void enable() { pimpl_->enable(); }

    /// \brief Disable the Signal.
    ///
    /// Connected Slots will _not_ be called when call operator is summoned.
    /// Safe when *this is already disabled.
    void disable() { pimpl_->disable(); }

   private:
    // Slots can track a Signals with a shared_ptr
    std::shared_ptr<Implementation> pimpl_;
};

template <typename Ret,
          typename... Args,
          typename Combiner,
          typename Group,
          typename Group_compare,
          typename Slot_function,
          typename Mutex>
const int
    Signal<Ret(Args...), Combiner, Group, Group_compare, Slot_function, Mutex>::
        arity;

}  // namespace sig
#endif  // SIGNAL_HPP

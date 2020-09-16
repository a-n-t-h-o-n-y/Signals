#ifndef SIGNALS_SLOT_HPP
#define SIGNALS_SLOT_HPP
#include <cstddef>
#include <iterator>
#include <memory>
#include <tuple>
#include <utility>

#include "expired_slot.hpp"
#include "signal_fwd.hpp"
#include "slot_base.hpp"
#include "slot_fwd.hpp"

namespace sig {

/// Represents a function that can be connected to a Signal.
/** Slots can track other objects to mirror their lifetime. If one tracked
 *  object is destroyed, *this will no longer be called by any Signal. Slots
 *  can be called like any other function, with operator(). Is _not_ nothrow
 *  move constructible/assignable because std::function is not.
 *  \param R Return type of the function.
 *  \param Args... Argument types to the function.
 *  \param FunctionType Internally held type where function will be stored. */
template <typename R, typename... Args, typename FunctionType>
class Slot<R(Args...), FunctionType> : public Slot_base {
   public:
    using Result_t        = R;
    using Argument_t      = std::tuple<Args...>;
    using Signature_t     = R(Args...);
    using Slot_function_t = FunctionType;

    /// Access to the type of argument number \p N.
    template <std::size_t N>
    using arg = typename std::tuple_element_t<N, Argument_t>;

    /// Number of arguments to the function.
    static constexpr auto arity = std::tuple_size_v<Argument_t>;

   public:
    /// Creates an empty Slot, throws std::bad_function_call if call attempted.
    Slot() = default;

    /// Construct from any type convertible to FunctionType.
    /** \p function Function pointer, lambda, functor, etc... to be stored. */
    template <typename F>
    Slot(F const& function) noexcept(noexcept(Slot_function_t{function}))
        : function_{function}
    {}

    /// Constructs from a Signal, automatically tracks the Signal.
    /// \param sig Signal stored in *this. Requires same signature as *this.
    template <typename T,
              typename U,
              typename V,
              typename W,
              typename X,
              typename Y>
    explicit Slot(Signal<T, U, V, W, X, Y> const& signal) : function_{signal}
    {
        track(signal);
    }

   public:
    /// Call underlying function if no tracked objects are expired.
    /** Passes on args... to the FunctionType object. No-op if any tracked
     *  objects no longer exist. Throws std::bad_function_call if Slot is empty.
     *  \param args... Arguments passed onto the underlying function.
     *  \param Result_t The value returned by the function call. */
    template <typename... Arguments>
    auto operator()(Arguments&&... args) const -> Result_t
    {
        if (this->expired())
            return Result_t();
        auto const l = this->lock();
        return function_(std::forward<Arguments>(args)...);
    }

    /// A throwing version of operator(), if any tracked objects are expired.
    template <typename... Arguments>
    auto call(Arguments&&... args) const -> Result_t
    {
        auto const l = this->expired() ? throw Expired_slot() : this->lock();
        return function_(std::forward<Arguments>(args)...);
    }

    /// Add a shared object to the tracked objects list.
    /** \param obj_ptr a weak_ptr to the object to be tracked.
     *  \returns Reference to *this. */
    auto track(std::weak_ptr<void> const& obj_ptr) -> Slot&
    {
        tracked_ptrs_.push_back(obj_ptr);
        return *this;
    }

    /// Add a Signal to the tracked objects list.
    /** \param sig Signal with same signature as *this.
     *  \returns Reference to *this. */
    template <typename T,
              typename U,
              typename V,
              typename W,
              typename X,
              typename Y>
    auto track(Signal<T, U, V, W, X, Y> const& sig) -> Slot&
    {
        tracked_ptrs_.push_back(sig.get_tracker());
        return *this;
    }

    /// Copies the tracked objects from slot to *this.
    /** Does not track slot itself.
     *  \param slot Slot to copy the tracked objects from.
     *  \returns Reference to *this. */
    auto track(Slot_base const& slot) -> Slot&
    {
        tracked_ptrs_.insert(std::begin(tracked_ptrs_),
                             std::begin(slot.get_tracked_container()),
                             std::end(slot.get_tracked_container()));
        return *this;
    }

    /// \returns Reference to the internally held FunctionType object.
    auto slot_function() -> Slot_function_t& { return function_; }

    /// \returns const reference to the internally held FunctionType object.
    auto slot_function() const -> Slot_function_t const& { return function_; }

   private:
    Slot_function_t function_;
};

}  // namespace sig
#endif  // SIGNALS_SLOT_HPP

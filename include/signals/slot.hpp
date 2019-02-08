/// \file
/// \brief Contains the definition of the Slot template class.
#ifndef SLOT_HPP
#define SLOT_HPP
#include <signals/expired_slot.hpp>
#include <signals/signal_fwd.hpp>
#include <signals/slot_base.hpp>
#include <signals/slot_fwd.hpp>

#include <functional>
#include <memory>
#include <tuple>
#include <vector>

namespace sig {

/// \brief Represents a function that can be connected to a Signal.
///
/// Slots can track other objects to mirror their lifetime. If one tracked
/// object is destroyed, *this will no longer be called by any Signal. Slots
/// can be called like any other function, with operator(). Is _not_ nothrow
/// move constructible/assignable because std::function is not.
/// \param Ret Return type of the function.
/// \param Args... Argument types to the function.
/// \param FunctionType Internally held type where the function will be stored.
template <typename Ret, typename... Args, typename FunctionType>
class Slot<Ret(Args...), FunctionType> : public Slot_base {
   public:
    using Result_t = Ret;
    using Argument_t = std::tuple<Args...>;
    using Signature_t = Ret(Args...);
    using Slot_function_t = FunctionType;

    /// Number of arguments to the function.
    static const int arity = std::tuple_size<Argument_t>::value;

    /// Access to the type of argument number \p n.
    template <unsigned n>
    class arg {
       public:
        using type = typename std::tuple_element<n, Argument_t>::type;
    };

    /// Creates an empty Slot, throws std::bad_function_call if call attempted.
    Slot() = default;

    /// \brief Construct from any type convertible to FunctionType.
    ///
    /// \param func Function pointer, lambda, functor, etc... to be stored.
    template <typename Function>
    Slot(const Function& func) : function_{func} {}

    /// \brief Constructs from a Signal, automatically tracks the Signal.
    ///
    /// \param sig Signal stored in *this. Requires same signature as *this.
    template <typename T,
              typename U,
              typename V,
              typename W,
              typename X,
              typename Y>
    explicit Slot(const Signal<T, U, V, W, X, Y>& sig)
        : function_{*(sig.lock_impl())} {
        track(sig);
    }

    /// \brief Call underlying function if no tracked objects are expired.
    ///
    /// Passes on args... to the FunctionType object. No-op if any tracked
    /// objects no longer exist. Throws std::bad_function_call if Slot is empty.
    /// \param args... Arguments passed onto the underlying function.
    /// \param Result_t The value returned by the function call.
    template <typename... Arguments>
    Result_t operator()(Arguments&&... args) const {
        if (this->expired()) {
            return Result_t();
        }
        auto l = this->lock();
        return function_(std::forward<Arguments>(args)...);
    }

    /// A throwing version of operator(), if any tracked objects are expired.
    template <typename... Arguments>
    Result_t call(Arguments&&... args) const {
        auto l = this->expired() ? throw Expired_slot() : this->lock();
        return function_(std::forward<Arguments>(args)...);
    }

    /// \brief Add a shared object to the tracked objects list.
    ///
    /// \param obj_ptr a weak_ptr to the object to be tracked.
    /// \returns Reference to *this.
    Slot& track(const std::weak_ptr<void>& obj_ptr) {
        tracked_ptrs_.push_back(obj_ptr);
        return *this;
    }

    /// \brief Add a Signal to the tracked objects list.
    ///
    /// \param sig Signal with same signature as *this.
    /// \returns Reference to *this.
    template <typename T,
              typename U,
              typename V,
              typename W,
              typename X,
              typename Y>
    Slot& track(const Signal<T, U, V, W, X, Y>& sig) {
        tracked_ptrs_.push_back(sig.lock_impl_as_void());
        return *this;
    }

    /// Copies the tracked objects from slot to *this.
    ///
    /// Does not track slot itself.
    /// \param slot Slot to copy the tracked objects from.
    /// \returns Reference to *this.
    Slot& track(const Slot_base& slot) {
        tracked_ptrs_.insert(std::begin(tracked_ptrs_),
                             std::begin(slot.get_tracked_container()),
                             std::end(slot.get_tracked_container()));
        return *this;
    }

    /// \returns Reference to the internally held FunctionType object.
    Slot_function_t& slot_function() { return function_; }

    /// \returns const reference to the internally held FunctionType object.
    const Slot_function_t& slot_function() const { return function_; }

   private:
    Slot_function_t function_;
};

template <typename Ret, typename... Args, typename FunctionType>
const int Slot<Ret(Args...), FunctionType>::arity;

}  // namespace sig
#endif  // SLOT_HPP

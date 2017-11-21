#ifndef DETAIL_CONNECTION_IMPL_HPP
#define DETAIL_CONNECTION_IMPL_HPP
#include <signals/detail/connection_impl_base.hpp>
#include <signals/slot.hpp>

#include <memory>
#include <utility>

namespace sig {
class Connection;

template <typename Signature>
class Connection_impl;

// Implementation class for Connection. This class owns the Slot involved in
// the connection. Inherits from Connection_impl_base, which implements shared
// connection block counts.
template <typename Ret, typename... Args>
class Connection_impl<Ret(Args...)> : public Connection_impl_base {
   public:
    using Extended_slot_t = Slot<Ret(const Connection&, Args...)>;

    Connection_impl() : slot_{}, connected_{false} {}

    explicit Connection_impl(Slot<Ret(Args...)> s)
        : slot_{std::move(s)}, connected_{true} {}

    // Constructs a Connection_impl with an extended slot and connection. This
    // binds the connection to the first parameter of the Extended_slot_t
    // function. The slot function type then matches the signal function type.
    // Then copies tracked items from the extended slot into the new slot.
    Connection_impl& emplace_extended(const Extended_slot_t& es,
                                      const Connection& c) {
        connected_ = true;
        slot_.slot_function() = [c, es](Args&&... args) {
            return es.slot_function()(c, std::forward<Args>(args)...);
        };
        for (const std::weak_ptr<void>& wp : es.get_tracked_container()) {
            slot_.track(wp);
        }
        return *this;
    }

    void disconnect() override { connected_ = false; }

    bool connected() const override { return connected_; }

    Slot<Ret(Args...)>& get_slot() { return slot_; }

    const Slot<Ret(Args...)>& get_slot() const { return slot_; }

   private:
    Slot<Ret(Args...)> slot_;
    bool connected_;
};

}  // namespace sig
#endif  // DETAIL_CONNECTION_IMPL_HPP

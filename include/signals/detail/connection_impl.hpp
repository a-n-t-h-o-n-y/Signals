#ifndef SIGNALS_DETAIL_CONNECTION_IMPL_HPP
#define SIGNALS_DETAIL_CONNECTION_IMPL_HPP
#include <memory>
#include <mutex>
#include <utility>

#include <signals/connection.hpp>
#include <signals/detail/connection_impl_base.hpp>
#include <signals/slot.hpp>

namespace sig {
class Connection;

template <typename Signature>
class Connection_impl;

// Implementation class for Connection. This class owns the Slot involved in
// the connection. Inherits from Connection_impl_base, which implements shared
// connection block counts.
template <typename R, typename... Args>
class Connection_impl<R(Args...)> : public Connection_impl_base {
   public:
    using Extended_slot_t = Slot<R(Connection const&, Args...)>;

    Connection_impl() : slot_{}, connected_{false} {}

    explicit Connection_impl(Slot<R(Args...)> s)
        : slot_{std::move(s)}, connected_{true}
    {}

   public:
    // Constructs a Connection_impl with an extended slot and connection. This
    // binds the connection to the first parameter of the Extended_slot_t
    // function. The slot function type then matches the signal function type.
    // Then copies tracked items from the extended slot into the new slot.
    auto emplace_extended(Extended_slot_t const& es, Connection const& c)
        -> Connection_impl&
    {
        auto const lock = std::lock_guard{mtx_};
        connected_      = true;

        slot_.slot_function() = [c, es](Args&&... args) {
            return es.slot_function()(c, std::forward<Args>(args)...);
        };
        for (std::weak_ptr<void> const& wp : es.get_tracked_container()) {
            slot_.track(wp);
        }
        return *this;
    }

    void disconnect() override
    {
        auto const lock = std::lock_guard{mtx_};
        connected_      = false;
    }

    auto connected() const -> bool override
    {
        auto const lock = std::lock_guard{mtx_};
        return connected_;
    }

    auto get_slot() -> Slot<R(Args...)>& { return slot_; }

    auto get_slot() const -> Slot<R(Args...)> const& { return slot_; }

   private:
    Slot<R(Args...)> slot_;
    bool connected_;
};

}  // namespace sig
#endif  // SIGNALS_DETAIL_CONNECTION_IMPL_HPP

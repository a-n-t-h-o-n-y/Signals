#ifndef SIGNALS_CONNECTION_HPP
#define SIGNALS_CONNECTION_HPP
#include <memory>
#include <utility>

#include <signals/detail/connection_impl_base.hpp>

namespace sig {

/// Represents the connection made when a Slot is connected to a Signal.
/** Can be queried to check if the Slot is still connected to the Signal. The
 *  Connection can be blocked by constructing a Shared_connection_block. */
class Connection {
   public:
    /// Default constructors a connection that refers to no real connection.
    Connection() = default;

    /// Constructor used by the Signal::connect() function.
    /** Signal uses this constructor to build Connection objects.
     *  Each Signal owns the Connection_impl, and each Connection_impl owns the
     *  connected Slot.
     *  \param wp_cib Implementation object that holds Connection's state.
     *  \sa Signal Slot */
    explicit Connection(std::weak_ptr<Connection_impl_base> wp_cib)
        : pimpl_{std::move(wp_cib)}
    {}

    /// Disconnects the connection.
    /** The Slot associated with this connection will no longer be called by the
     *  associated Signal. If connection is already disconnected, this is a
     *  no-op. */
    void disconnect() const
    {
        if (!this->pimpl_.expired())
            pimpl_.lock()->disconnect();
    }

    /// Query whether the connection is connected or not.
    /** \returns True if *this is connected, false otherwise. */
    auto connected() const -> bool
    {
        if (!this->pimpl_.expired())
            return pimpl_.lock()->connected();
        return false;
    }

    /// Query whether the connection is currently blocked or not.
    /** Blocking can happen from initializing a Shared_connection_block with
     *  *this.
     *  \returns True if the connection is blocked, false otherwise. */
    auto blocked() const -> bool
    {
        if (!this->pimpl_.expired())
            return pimpl_.lock()->blocked();
        return false;
    }

    /// Return true if both parameters refer to the same Signal/Slot connection.
    auto operator==(Connection const& x) const -> bool
    {
        return pimpl_.lock() == x.pimpl_.lock();
    }

    /// Does pointer less than comparison of underlying implementation.
    auto operator<(Connection const& x) const -> bool
    {
        return pimpl_.lock() < x.pimpl_.lock();
    }

    friend class Shared_connection_block;

   private:
    std::weak_ptr<Connection_impl_base> pimpl_;
};

/// Returns !(*this == x)
inline auto operator!=(Connection const& x, Connection const& y) -> bool
{
    return !(x == y);
}

}  // namespace sig
#endif  // SIGNALS_CONNECTION_HPP

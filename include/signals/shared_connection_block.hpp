#ifndef SIGNALS_SHARED_CONNECTION_BLOCK_HPP
#define SIGNALS_SHARED_CONNECTION_BLOCK_HPP
#include <memory>

#include "connection.hpp"

namespace sig {
class Connection_impl_base;

/// Blocks a Signal/Slot Connection.
/** Any number of Shared_connection_blocks can be built on a single connection,
 *  when the last block goes out of scope, the connection is unblocked. */
class Shared_connection_block {
   public:
    /// Create a Shared_connection_block from a Connection and a boolean.
    /** Default constructs to blocking an empty Connection.
     *  \param conn The Connection to be blocked by *this.
     *  \param initially_block If true, *this blocks the Connection, otherwise
     *  *this only holds a reference to the Connection and can block the
     *  Connection at some other time by calling block(). */
    explicit Shared_connection_block(Connection const& conn = Connection{},
                                     bool initially_block   = true)
        : connection_{conn.pimpl_}, blocking_{initially_block}
    {
        if (this->active())
            connection_.lock()->add_block();
    }

    /// Creates a copy of \p x, increasing the block count on the
    /** Connection if \p x is blocking. */
    Shared_connection_block(Shared_connection_block const& x)
        : connection_{x.connection_}, blocking_{x.blocking_}
    {
        if (this->active())
            connection_.lock()->add_block();
    }

    /// Reset *this' Connection to point to \p x's Connection.
    /** If *this was blocking, then the block count is decremented on the
     *  original Connection. If \p x is blocking, then \p x's Connection gets
     *  an additional block. */
    auto operator=(Shared_connection_block const& x) -> Shared_connection_block&
    {
        if (this == &x)
            return *this;
        this->reset(x);
        return *this;
    }

    /// Releases this block from the Connection.
    /** Connection may be blocked by other Shared_connection_block objects. */
    ~Shared_connection_block() { this->unblock(); }

    /// Releases the Connection block. No-op if not currently blocking.
    void unblock()
    {
        if (this->active()) {
            connection_.lock()->remove_block();
            blocking_ = false;
        }
    }

    /// Reasserts a block on a Connection. No-op if currently blocking.
    void block()
    {
        if (!connection_.expired() && !blocking_) {
            connection_.lock()->add_block();
            blocking_ = true;
        }
    }

    /// \returns True if *this is currently blocking a Connection, else false.
    auto blocking() const -> bool
    {
        return !connection_.expired() && blocking_;
    }

    /// \returns The Connection object associated with *this.
    auto connection() const -> Connection { return Connection(connection_); }

   private:
    std::weak_ptr<Connection_impl_base> connection_;
    bool blocking_;

   private:
    // Remove the block to the associated Connection and reset with the contents
    // of \p x, applying a block to the new Connection if x is blocking.
    void reset(Shared_connection_block const& x)
    {
        this->unblock();
        connection_ = x.connection_;
        blocking_   = x.blocking_;
        if (this->active())
            connection_.lock()->add_block();
    }

    // Return true if the connection pointed to is still alive and *this is
    // currently blocking.
    auto active() -> bool { return !connection_.expired() && blocking_; }
};

}  // namespace sig
#endif  // SIGNALS_SHARED_CONNECTION_BLOCK_HPP

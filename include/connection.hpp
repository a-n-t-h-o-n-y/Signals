/// \file
/// \brief Contains the definition of the Connection class.
#ifndef CONNECTION_HPP
#define CONNECTION_HPP

#include "detail/connection_impl_base.hpp"

#include <memory>

namespace mcurses {

class Connection {
   public:
    Connection() = default;

    // Signal uses this constructor to build Connection objects.
    // Each Signal owns the Connection_impl, and each Connection_impl owns the
    // connecte Slot.
    explicit Connection(std::weak_ptr<Connection_impl_base> wp_cib)
        : pimpl_{std::move(wp_cib)} {}

    void disconnect() const {
        if (!this->pimpl_.expired()) {
            pimpl_.lock()->disconnect();
        }
    }

    bool connected() const {
        if (!this->pimpl_.expired()) {
            return pimpl_.lock()->connected();
        }
        return false;
    }

    bool blocked() const {
        if (!this->pimpl_.expired()) {
            return pimpl_.lock()->blocked();
        }
        return false;
    }

    bool operator==(const Connection& x) {
        return pimpl_.lock() == x.pimpl_.lock();
    }

    bool operator!=(const Connection& x) { return !(*this == x); }

    bool operator<(const Connection& x) {
        return pimpl_.lock() < x.pimpl_.lock();
    }

    friend class shared_connection_block;

   private:
    std::weak_ptr<Connection_impl_base> pimpl_;
};

}  // namespace mcurses

#endif  // CONNECTION_HPP

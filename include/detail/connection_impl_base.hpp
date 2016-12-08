#ifndef CONNECTION_IMPL_BASE_HPP
#define CONNECTION_IMPL_BASE_HPP

#include <cstddef>

// Forward Declarations
namespace mcurses {
class Connection;
} // namespace mcurses

namespace mcurses {

// Access to connections without template parameters. This is the interface for
// the Connection_impl class template. This is used by the connnecton class as
// a pointer with a concrete Connection_impl instantiation.
class Connection_impl_base {
   public:
    virtual ~Connection_impl_base() = default;

    virtual void disconnect() = 0;

    virtual bool connected() const = 0;

    inline bool blocked() const {
        return blocking_object_count_ < 1 ? false : true;
    }

    inline void add_block() {
        ++blocking_object_count_;
    }
    inline void remove_block() {
        --blocking_object_count_;
    }

   protected:
    // Connections keep track of how many objects are blocking the connection.
    std::size_t blocking_object_count_ = 0;
};

}  // namespace mcurses

#endif  // CONNECTION_IMPL_BASE_HPP

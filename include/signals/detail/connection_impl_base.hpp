#ifndef DETAIL_CONNECTION_IMPL_BASE_HPP
#define DETAIL_CONNECTION_IMPL_BASE_HPP
#include <cstddef>

namespace sig {

// Provides an interface for the Connection class to hold a pointer to a
// non-templated implementation. A Connection can remain non-templated, while
// having an internal implementation vary on the Slot type.
class Connection_impl_base {
   public:
    Connection_impl_base() = default;
    Connection_impl_base(const Connection_impl_base&) = default;
    Connection_impl_base& operator=(const Connection_impl_base&) = default;
    Connection_impl_base(Connection_impl_base&&) = default;
    Connection_impl_base& operator=(Connection_impl_base&&) = default;
    virtual ~Connection_impl_base() = default;

    virtual void disconnect() = 0;

    virtual bool connected() const = 0;

    inline bool blocked() const {
        return blocking_object_count_ < 1 ? false : true;
    }

    inline void add_block() { ++blocking_object_count_; }
    inline void remove_block() { --blocking_object_count_; }

   protected:
    std::size_t blocking_object_count_ = 0;
};

}  // namespace sig
#endif  // DETAIL_CONNECTION_IMPL_BASE_HPP

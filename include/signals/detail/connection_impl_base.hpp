#ifndef DETAIL_CONNECTION_IMPL_BASE_HPP
#define DETAIL_CONNECTION_IMPL_BASE_HPP
#include <cstddef>
#include <mutex>

namespace sig {

// Provides an interface for the Connection class to hold a pointer to a
// non-templated implementation. A Connection can remain non-templated, while
// having an internal implementation vary on the Slot type.
class Connection_impl_base {
   public:
    Connection_impl_base() = default;
    Connection_impl_base(const Connection_impl_base& other) {
        std::lock_guard<Mutex> lock{other.mtx_};
        blocking_object_count_ = other.blocking_object_count_;
    }

    Connection_impl_base(Connection_impl_base&& other) {
        std::lock_guard<Mutex> lock{other.mtx_};
        blocking_object_count_ = std::move(other.blocking_object_count_);
    }

    Connection_impl_base& operator=(const Connection_impl_base& rhs) {
        if (this != &rhs) {
            std::unique_lock<Mutex> lhs_lock{this->mtx_, std::defer_lock};
            std::unique_lock<Mutex> rhs_lock{rhs.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            blocking_object_count_ = rhs.blocking_object_count_;
        }
        return *this;
    }

    Connection_impl_base& operator=(Connection_impl_base&& rhs) {
        if (this != &rhs) {
            std::unique_lock<Mutex> lhs_lock{this->mtx_, std::defer_lock};
            std::unique_lock<Mutex> rhs_lock{rhs.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            blocking_object_count_ = std::move(rhs.blocking_object_count_);
        }
        return *this;
    }

    virtual ~Connection_impl_base() = default;

    virtual void disconnect() = 0;

    virtual bool connected() const = 0;

    inline bool blocked() const {
        std::lock_guard<Mutex> lock{mtx_};
        return blocking_object_count_ < 1 ? false : true;
    }

    inline void add_block() {
        std::lock_guard<Mutex> lock{mtx_};
        ++blocking_object_count_;
    }

    inline void remove_block() {
        std::lock_guard<Mutex> lock{mtx_};
        --blocking_object_count_;
    }

   protected:
    std::size_t blocking_object_count_ = 0;

    using Mutex = std::mutex;
    mutable Mutex mtx_;
};

}  // namespace sig
#endif  // DETAIL_CONNECTION_IMPL_BASE_HPP

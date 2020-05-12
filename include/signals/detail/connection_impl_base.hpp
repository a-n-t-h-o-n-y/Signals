#ifndef SIGNALS_DETAIL_CONNECTION_IMPL_BASE_HPP
#define SIGNALS_DETAIL_CONNECTION_IMPL_BASE_HPP
#include <cstddef>
#include <mutex>
#include <utility>

namespace sig {

// Provides an interface for the Connection class to hold a pointer to a
// non-templated implementation. A Connection can remain non-templated, while
// having an internal implementation vary on the Slot type.
class Connection_impl_base {
   public:
    virtual ~Connection_impl_base() = default;

    Connection_impl_base() = default;

    Connection_impl_base(Connection_impl_base const& other)
    {
        auto const lock        = std::lock_guard{other.mtx_};
        blocking_object_count_ = other.blocking_object_count_;
    }

    Connection_impl_base(Connection_impl_base&& other)
    {
        auto const lock        = std::lock_guard{other.mtx_};
        blocking_object_count_ = std::move(other.blocking_object_count_);
    }

    auto operator=(Connection_impl_base const& rhs) -> Connection_impl_base&
    {
        if (this != &rhs) {
            auto lhs_lock = std::unique_lock{this->mtx_, std::defer_lock};
            auto rhs_lock = std::unique_lock{rhs.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            blocking_object_count_ = rhs.blocking_object_count_;
        }
        return *this;
    }

    auto operator=(Connection_impl_base&& rhs) -> Connection_impl_base&
    {
        if (this != &rhs) {
            auto lhs_lock = std::unique_lock{this->mtx_, std::defer_lock};
            auto rhs_lock = std::unique_lock{rhs.mtx_, std::defer_lock};
            std::lock(lhs_lock, rhs_lock);
            blocking_object_count_ = std::move(rhs.blocking_object_count_);
        }
        return *this;
    }

   public:
    virtual void disconnect() = 0;

    virtual auto connected() const -> bool = 0;

    auto blocked() const -> bool
    {
        auto const lock = std::lock_guard{mtx_};
        return blocking_object_count_ < 1 ? false : true;
    }

    void add_block()
    {
        auto const lock = std::lock_guard{mtx_};
        ++blocking_object_count_;
    }

    void remove_block()
    {
        auto const lock = std::lock_guard{mtx_};
        --blocking_object_count_;
    }

   protected:
    std::size_t blocking_object_count_ = 0;

    mutable std::mutex mtx_;
};

}  // namespace sig
#endif  // SIGNALS_DETAIL_CONNECTION_IMPL_BASE_HPP

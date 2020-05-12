#ifndef SIGNALS_SLOT_BASE_HPP
#define SIGNALS_SLOT_BASE_HPP
#include <algorithm>
#include <iterator>
#include <memory>
#include <vector>

namespace sig {

/// Base class for Slot, handles tracked objects.
class Slot_base {
   public:
    using Locked_container_t  = std::vector<std::shared_ptr<void>>;
    using Tracked_container_t = std::vector<std::weak_ptr<void>>;

   public:
    Slot_base()                 = default;
    Slot_base(Slot_base const&) = default;
    Slot_base& operator=(Slot_base const&) = default;
    Slot_base(Slot_base&&)                 = default;
    Slot_base& operator=(Slot_base&&) = default;
    virtual ~Slot_base()              = default;

   public:
    /// \returns True if any tracked object has been destroyed.
    auto expired() const -> bool
    {
        return std::any_of(
            std::begin(tracked_ptrs_), std::end(tracked_ptrs_),
            [](auto const& tracked) { return tracked.expired(); });
    }

    /// Locks the tracked objects so they cannot be destroyed.
    /** \returns Container of locked objects, as long as this container is alive
     *  all of the tracked objects will be alive, as long as they have not
     *  already expired. */
    auto lock() const -> Locked_container_t
    {
        auto locked_vec = Locked_container_t{};
        for (auto& tracked : tracked_ptrs_)
            locked_vec.push_back(tracked.lock());
        return locked_vec;
    }

    /// \returns The internally held container of tracked objects.
    auto get_tracked_container() const -> Tracked_container_t const&
    {
        return tracked_ptrs_;
    }

   protected:
    Tracked_container_t tracked_ptrs_;
};

}  // namespace sig
#endif  // SIGNALS_SLOT_BASE_HPP

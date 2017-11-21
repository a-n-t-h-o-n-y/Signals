/// \file
/// \brief Contains definition of Slot_base class.
#ifndef SLOT_BASE_HPP
#define SLOT_BASE_HPP
#include <memory>
#include <vector>

namespace sig {

/// \brief Base class for Slot, handles tracked objects.
class Slot_base {
   public:
    using Locked_container_t = std::vector<std::shared_ptr<void>>;
    using Tracked_container_t = std::vector<std::weak_ptr<void>>;

    Slot_base() = default;
    Slot_base(const Slot_base&) = default;
    Slot_base& operator=(const Slot_base&) = default;
    Slot_base(Slot_base&&) = default;
    Slot_base& operator=(Slot_base&&) = default;
    virtual ~Slot_base() = default;

    /// \returns True if any tracked object has been destroyed.
    bool expired() const {
        for (auto& tracked : tracked_ptrs_) {
            if (tracked.expired()) {
                return true;
            }
        }
        return false;
    }

    /// \brief Locks the tracked objects so they cannot be destroyed.
    ///
    /// \returns Container of locked objects, as long as this container is alive
    /// all of the tracked objects will be alive, as long as they have not
    /// already expired.
    Locked_container_t lock() const {
        Locked_container_t locked_vec;
        for (auto& tracked : tracked_ptrs_) {
            locked_vec.push_back(tracked.lock());
        }
        return locked_vec;
    }

    /// \returns The internally held container of tracked objects.
    const Tracked_container_t& get_tracked_container() const {
        return tracked_ptrs_;
    }

   protected:
    Tracked_container_t tracked_ptrs_;
};

}  // namespace sig
#endif  // SLOT_BASE_HPP

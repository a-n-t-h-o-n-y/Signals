#ifndef SIGNALS_EXPIRED_SLOT_HPP
#define SIGNALS_EXPIRED_SLOT_HPP
#include <stdexcept>

/// \namespace
/// Signals Library namespace.
namespace sig {

/// Thrown when a Slot is accessed after it has expired.
struct Expired_slot : std::logic_error {
    explicit Expired_slot() : logic_error{"Slot is not valid."} {}
};

}  // namespace sig
#endif  // SIGNALS_EXPIRED_SLOT_HPP

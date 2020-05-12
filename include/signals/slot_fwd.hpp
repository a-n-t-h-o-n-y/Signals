/// \file
/// Contains the forward declaration of the Slot class template.
/** Use this header instead of trying to forward declare the class yourself. */
#ifndef SIGNALS_SLOT_FWD_HPP
#define SIGNALS_SLOT_FWD_HPP
#include <functional>

namespace sig {

template <typename Signature, typename FunctionType = std::function<Signature>>
class Slot;

}  // namespace sig
#endif  // SIGNALS_SLOT_FWD_HPP

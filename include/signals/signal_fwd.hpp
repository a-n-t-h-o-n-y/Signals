/// \file
/// Contains the forward declaration of Signal.
/** Use this header instead of trying to forward declare the class yourself. */
#ifndef SIGNALS_SIGNAL_FWD_HPP
#define SIGNALS_SIGNAL_FWD_HPP
#include <functional>
#include <mutex>

#include <signals/detail/function_type_splitter.hpp>
#include <signals/optional_last_value.hpp>

namespace sig {

template <typename Signature,
          typename Combiner = sig::Optional_last_value<
              typename sig::Function_type_splitter<Signature>::Return_t>,
          typename Group         = int,
          typename Group_compare = std::less<Group>,
          typename Slot_function = std::function<Signature>,
          typename Mutex         = std::mutex>
class Signal;

}  // namespace sig
#endif  // SIGNALS_SIGNAL_FWD_HPP

/// \file
/// \brief Contains the foward declaration of Signal. Use this header instead of
/// trying to forward declare the class yourself.
#ifndef SIGNAL_FWD_HPP
#define SIGNAL_FWD_HPP

#include <mutex>
#include <functional>

#include "optional_last_value.hpp"

#include "detail/function_type_splitter.hpp"

namespace mcurses {

template <typename Signature,
          typename Combiner = mcurses::Optional_last_value<
              typename mcurses::Function_type_splitter<Signature>::return_type>,
          typename Group = int,
          typename GroupCompare = std::less<Group>,
          typename SlotFunction = std::function<Signature>,
          typename Mutex = std::mutex>
class Signal;

}  // namespace mcurses

#endif  // SIGNAL_FWD_HPP

#ifndef SIGNALS_DETAIL_FUNCTION_TYPE_SPLITTER_HPP
#define SIGNALS_DETAIL_FUNCTION_TYPE_SPLITTER_HPP
#include <tuple>

namespace sig {

template <typename Function>
class Function_type_splitter;

// For extracting the return and argument types from a function.
template <typename R, typename... Args>
class Function_type_splitter<R(Args...)> {
   public:
    static constexpr auto arity = sizeof...(Args);

    using Return_t         = R;
    using Argument_tuple_t = std::tuple<Args...>;
};

}  // namespace sig
#endif  // SIGNALS_DETAIL_FUNCTION_TYPE_SPLITTER_HPP

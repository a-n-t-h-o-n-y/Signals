#ifndef SIGNALS_OPTIONAL_LAST_VALUE_HPP
#define SIGNALS_OPTIONAL_LAST_VALUE_HPP
#include <optional>
#include <utility>

namespace sig {

/// A functor class that returns the last value in an iterator range.
/** Attempts to dereference every iterator in the range [first, last).
 *  If the iterator range is empty, an empty opt::Optional object is returned,
 *  otherwise an opt::Optional wrapping the last value is returned. */
template <typename T>
class Optional_last_value {
   public:
    /// Type of object the iterator range points to.
    using Result_type = std::optional<T>;

   public:
    /// \param  first  Input iterator to the first element in the range.
    /// \param  last   Input iterator to one past the last element in the range.
    /// \returns       The value stored in the last iterator of the range,
    ///                wrapped in an opt::Optional.
    template <typename InputIterator>
    auto operator()(InputIterator first, InputIterator last) const
        -> Result_type
    {
        if (first == last)
            return {std::nullopt};
        auto temp = T();
        while (first != last) {
            temp = *first;
            ++first;
        }
        return {std::move(temp)};
    }
};

/// Specialization for void return type.
/** Useful when dereferencing of iterators produces side effects. */
template <>
class Optional_last_value<void> {
   public:
    /// Type of object the iterator range points to.
    using Result_type = void;

   public:
    /// \param  first  Input iterator to the first element in the range.
    /// \param  last   Input iterator to one past the last element in the range.
    template <typename InputIterator>
    auto operator()(InputIterator first, InputIterator last) const
        -> Result_type
    {
        while (first != last) {
            *first;
            ++first;
        }
    }
};

}  // namespace sig
#endif  // SIGNALS_OPTIONAL_LAST_VALUE_HPP

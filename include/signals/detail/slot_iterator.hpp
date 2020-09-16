#ifndef SIGNALS_DETAIL_SLOT_ITERATOR_HPP
#define SIGNALS_DETAIL_SLOT_ITERATOR_HPP
#include <iterator>

namespace sig {

// Slot_iterator points to a functor object, when it is dereferenced you get a
// function type object that is callable with no arguments. The iterator is
// useful for iterating over a container of functions and calling each function
// on a dereference operation. InputIterator is any container iterator that
// holds a callable type and can be incremented, dereferenced and copied.
template <typename InputIterator>
class Slot_iterator {
   public:
    using Iter_value_t =
        typename std::iterator_traits<InputIterator>::value_type;
    using Result_t = typename Iter_value_t::result_type;

   public:
    Slot_iterator() = default;

    explicit Slot_iterator(InputIterator iter) : iter_{iter} {}

   public:
    auto operator*() -> Result_t
    {
        auto slot = *iter_;
        return slot();
    }

    auto operator++() -> Slot_iterator&
    {
        ++iter_;
        return *this;
    }

    auto operator==(Slot_iterator const& x) -> bool { return iter_ == x.iter_; }

    auto operator!=(Slot_iterator const& x) -> bool { return !operator==(x); }

   private:
    InputIterator iter_;
};

}  // namespace sig

#endif  // SIGNALS_DETAIL_SLOT_ITERATOR_HPP

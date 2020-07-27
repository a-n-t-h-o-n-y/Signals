# Signals

![build badge](https://github.com/a-n-t-h-o-n-y/Signals/workflows/build/badge.svg)

An implementation of the Boost::Signals2 library.

The Signals Library is an implementation of the Observer pattern, an event
driven callback system. Calling a Signal will call each of its registered
Slots(functions). Some features:

- Connect any number of Slots to a Signal.
- Signals propagate their given arguments to each of their Slots.
- Connection management with connection block objects and object lifetime
  tracking, allowing Slots to disconnect when a tracked object is destroyed.
- Signal's return value is determined by a Combiner, which establishes how
  to return a single value from all of the individual Slots' return values.
- Thread-safe

Reference docs can be found [here](https://a-n-t-h-o-n-y.github.io/Signals/).

## Code Examples

#### Basic Use

```cpp
// Create Signal and Connect Two Slots
auto s = sig::Signal<void(int)>{};
s.connect([](int i) { std::cout << "value: " << i << '\n'; });
s.connect([](int i) { std::cout << "double: " << i * 2 << '\n'; });

// Connecting with Group Numbers
s.connect(1, [](int i) { std::cout << "half: " << i / 2 << '\n'; });
s.connect(1, [](int i) { std::cout << "quarter: " << i / 4 << '\n'; },
               sig::Position::at_front);
// Emit Signal -- Calls each connected Slot, in expected order.
s(5);
```

#### Connection Management

```cpp
auto s = sig::Signal<void()>{};
sig::Connection c = s.connect([] { std::cout << "Hello, World!\n"; });
assert(c.connected());
{
    // Connection is blocked as long as a Shared_connection_block is alive.
    auto const block = sig::Shared_connection_block{c};
    assert(c.blocked());
    assert(c.connected());
}
assert(!c.blocked());
s();
c.disconnect();
assert(!c.connected());
s();
```

#### Object Lifetime Tracking

```cpp
auto to_track = std::make_shared<int>(5);
auto slot = sig::Slot<void()>{[] { std::cout << "to_track is still alive.\n"; }};
slot.track(to_track);
auto s = sig::Signal<void()>{};
s.connect(slot);
s();   // Outputs: "to_track is still alive."
to_track.reset();
s();   // No Output
```

#### Extended Slots

```cpp
// Slot can disconnect itself from its own Connection.
auto ext_slot = sig::Slot<int(sig::Connection, int)>{[](sig::Connection c, int i) {
    c.disconnect();
    return i;
}};
auto s = sig::Signal<int(int)>{};
s.connect_extended(ext_slot);
assert(s.num_slots() == 1);
s(9);
assert(s.num_slots() == 0);
```

#### User Defined Combiner

```cpp
// Sum Combiner, returns a std::optional, which if initialized contains the sum
// of all return values from connected Slots. For arithmetic types only.
template <typename T>
class Optional_sum {
   public:
    using result_type = std::optional<T>;

    template <typename InputIterator>
    auto operator()(InputIterator first, InputIterator last) const -> result_type {
        if (first == last)
            return std::nullopt;
        auto sum = static_cast<T>(0);
        while (first != last) {
            sum += *first++;
            ++first;
        }
        return sum;
    }
};

// Later on...
auto s = sig::Signal<int(int), Optional_sum<int>>{};
s.connect([](int i) { return i; });
s.connect([](int i) { return i * 2; });
s.connect([](int i) { return i * 3; });
s.connect([](int i) { return i * 4; });
s.connect([](int i) { return i * 5; });

std::optional<int> result = s(10);
assert(result);
assert(*result == 150);
```

## Installation

Using CMake:

```sh
git clone https://github.com/a-n-t-h-o-n-y/Signals.git
mkdir Signals/build && cd Signals/build
git submodule update --init --recursive   # For testing only
cmake ..
make signals_test       # Build tests(Optional)
ctest                   # Run tests(Optional)
sudo make install       # Install headers to system default include directory
```

## Tests

Signals uses catch-2. The tests have a dependency on boost::function.

## License

This software is distributed under the [MIT License](LICENSE.txt).

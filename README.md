![build badge](https://github.com/a-n-t-h-o-n-y/Signals/workflows/build/badge.svg)

## Overview
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


## Code Examples
#### Basic Use
```cpp
// Create Signal and Connect Two Slots
sig::Signal<void(int)> my_sig;
my_sig.connect([](int i) { std::cout << "value: " << i << '\n'; });
my_sig.connect([](int i) { std::cout << "double: " << i * 2 << '\n'; });

// Connecting with Group Numbers
my_sig.connect(1, [](int i) { std::cout << "half: " << i / 2 << '\n'; });
my_sig.connect(1, [](int i) { std::cout << "quarter: " << i / 4 << '\n'; },
               sig::Position::at_front);
// Emit Signal
my_sig(5);
```
#### Connection Management
```cpp
sig::Signal<void()> my_sig;
sig::Connection c = my_sig.connect([] { std::cout << "Hello, World!\n"; });
assert(c.connected());
{
    // Connection is blocked as long as a Shared_connection_block is alive.
    sig::Shared_connection_block scb{c};
    assert(c.blocked());
    assert(c.connected());
}
assert(!c.blocked());
my_sig();
c.disconnect();
assert(!c.connected());
my_sig();
```
#### Object Lifetime Tracking
```cpp
auto to_track = std::make_shared<int>(5);
sig::Slot<void()> slot{[] { std::cout << "to_track is still alive.\n"; }};
slot.track(to_track);
sig::Signal<void()> my_sig;
my_sig.connect(slot);
my_sig();   // Outputs: "to_track is still alive."
to_track.reset();
my_sig();   // No Output
```
#### Extended Slots
```cpp
// Slot can disconnect itself from its own Connection.
sig::Slot<int(sig::Connection, int)> ext_slot{[](sig::Connection c, int i) {
    c.disconnect();
    return i;
}};
sig::Signal<int(int)> my_sig;
my_sig.connect_extended(ext_slot);
assert(my_sig.num_slots() == 1);
my_sig(9);
assert(my_sig.num_slots() == 0);
```
#### User Defined Combiner
```cpp
// Sum Combiner, returns an Optional, which if initialized contains the sum of
// all return values from connected Slots. For arithmetic types only.
template <typename T>
class Optional_sum {
   public:
    using result_type = opt::Optional<T>;
    template <typename InputIterator>
    result_type operator()(InputIterator first, InputIterator last) const {
        if (first == last) {
            return result_type{opt::none};
        }
        T sum{static_cast<T>(0)};
        while (first != last) {
            sum += *first;
            ++first;
        }
        return result_type{sum};
    }
};

// Later on...
sig::Signal<int(int), Optional_sum<int>> my_sig;
my_sig.connect([](int i) { return i; });
my_sig.connect([](int i) { return i * 2; });
my_sig.connect([](int i) { return i * 3; });
my_sig.connect([](int i) { return i * 4; });
my_sig.connect([](int i) { return i * 5; });

opt::Optional<int> result = my_sig(10);
assert(result);
assert(*result == 150);
```

## Motivation
This implementation was created for use in
[CPPurses](https://github.com/a-n-t-h-o-n-y/CPPurses).

## Installation
Using CMake:
```
git clone https://github.com/a-n-t-h-o-n-y/Signals.git
mkdir Signals/build && cd Signals/build
git submodule update --init --recursive   # For testing only
cmake ..
make signals_test       # Build tests(Optional)
ctest                   # Run tests(Optional)
sudo make install       # Install headers to system default include directory
```

## Documentation
Doxygen documentation can be found [here](
https://a-n-t-h-o-n-y.github.io/Signals/).

## Tests
Signals uses google test.

## License
This software is distributed under the [MIT License](LICENSE.txt).

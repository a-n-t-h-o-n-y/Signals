#include <memory>
#include <vector>

#include <catch2/catch.hpp>

#include <signals/slot.hpp>
#include <signals/slot_base.hpp>

using sig::Slot;

TEST_CASE("Slot_base::expired()", "[slot_base]")
{
    Slot<void(int)> s = {[](int) { return; }};
    CHECK_FALSE(s.expired());

    auto s_char = std::make_shared<char>('h');
    s.track(s_char);
    CHECK_FALSE(s.expired());

    s.track(std::make_shared<int>(5));
    CHECK(s.expired());

    Slot<void(int)> s2 = {[](int) { return; }};
    auto s_int         = std::make_shared<int>(7);
    auto s_char2       = std::make_shared<char>('y');
    s2.track(s_int);
    s2.track(s_char2);
    CHECK_FALSE(s2.expired());
    s_int.reset();
    CHECK(s2.expired());

    auto s_dbl = std::make_shared<double>(2.5);
    s2.track(s_dbl);
    CHECK(s2.expired());
}

// Add Tracked items and check again
TEST_CASE("Slot_base::lock()", "[slot_base]")
{
    Slot<void(int)> s = {[](int) { return; }};
    CHECK(std::vector<std::shared_ptr<void>>{} == s.lock());

    auto s_char = std::make_shared<char>('h');
    s.track(s_char);

    CHECK(1 == s.lock().size());

    s.track(std::make_shared<double>(5));

    CHECK(2 == s.lock().size());

    CHECK(s.expired());
}

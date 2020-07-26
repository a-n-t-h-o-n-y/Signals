#include <functional>
#include <memory>
#include <optional>
#include <typeinfo>

#include <boost/function.hpp>

#include <signals/expired_slot.hpp>
#include <signals/signal.hpp>
#include <signals/slot.hpp>

#include <catch2/catch.hpp>

using sig::Expired_slot;
using sig::Signal;
using sig::Slot;

TEST_CASE("Slot::arity()", "[slot]")
{
    Slot<void(int, int, double, float)> a_slot = {
        [](int, int, double, float) { return; }};

    CHECK(4 == a_slot.arity);

    CHECK(3 == Slot<int(char, int, double)>::arity);
}

TEST_CASE("Slot::arg<> access", "[slot]")
{
    using type_zero = Slot<long(int, double, float, float, char, bool)>::arg<0>;
    using type_one  = Slot<long(int, double, float, float, char, bool)>::arg<1>;
    using type_two  = Slot<long(int, double, float, float, char, bool)>::arg<2>;
    using type_three =
        Slot<long(int, double, float, float, char, bool)>::arg<3>;
    using type_four = Slot<long(int, double, float, float, char, bool)>::arg<4>;
    using type_five = Slot<long(int, double, float, float, char, bool)>::arg<5>;
    CHECK(typeid(int) == typeid(type_zero));
    CHECK(typeid(double) == typeid(type_one));
    CHECK(typeid(float) == typeid(type_two));
    CHECK(typeid(float) == typeid(type_three));
    CHECK(typeid(char) == typeid(type_four));
    CHECK(typeid(bool) == typeid(type_five));
}

TEST_CASE("Slot()", "[slot]")
{
    Slot<void(int)> s;
    CHECK(s.slot_function() == nullptr);
}

TEST_CASE("Slot::operator=(Slot const&)", "[slot]")
{
    Slot<int(int, double)> s;
    Slot<int(int, double)> s2{[](int, double) { return 3; }};

    auto i = std::make_shared<int>(6);

    s2.track(i);

    s = s2;

    CHECK(3 == s(6, 4.3));

    CHECK_FALSE(s.expired());

    i.reset();

    CHECK(s.expired());
}

TEST_CASE("Slot with default function type", "[slot]")
{
    Slot<double(int, double)> a_slot = {[](int i, double d) { return i + d; }};

    CHECK(7.2 == Approx(a_slot(4, 3.2)).margin(1e-12));
}

TEST_CASE("Slot with boost function type", "[slot]")
{
    Slot<int(int), boost::function<int(int)>> a_slot{
        [](int i) { return i * 2; }};

    CHECK(4 == a_slot(2));
}

// Update with checks once implemented(?)
TEST_CASE("Slot::track(weak_ptr)", "[slot]")
{
    Slot<void(int, double)> s = {[](int, double) { return; }};
    auto s_i                  = std::make_shared<int>(5);
    s.track(s_i);
    s.track(std::make_shared<char>('g'));
}

TEST_CASE("Slot::track(Signal)", "[slot]")
{
    auto sig = std::make_shared<Signal<void(int)>>();
    sig->connect([](int) { return; });

    Slot<char(int, double)> slt = [](int, double) { return 'k'; };

    slt.track(*sig);

    Signal<char(int, double)> sig2;

    sig2.connect(slt);

    REQUIRE(sig2(1, 1.0));
    CHECK('k' == *sig2(1, 1.0));

    CHECK_FALSE(slt.expired());

    sig.reset();

    CHECK(slt.expired());
    // EXPECT_THROW(sig2(1, 1.0), Expired_slot);
    CHECK_NOTHROW(sig2(1, 1.0));
}

TEST_CASE("Slot::track(Slot)", "[slot]")
{
    Slot<void(int)> s1         = [](int) { return; };
    Slot<void(int)> s2         = [](int) { return; };
    Slot<double(char, int)> s3 = [](char, int) { return 3.4; };
    Slot<void(int)> s4         = [](int) { return; };

    auto obj1 = std::make_shared<int>(5);
    s1.track(obj1);

    auto obj2 = std::make_shared<char>('k');
    s2.track(obj2);

    CHECK_FALSE(s1.expired());
    CHECK_FALSE(s2.expired());
    CHECK_FALSE(s3.expired());

    s3.track(s1);
    CHECK_FALSE(s1.expired());
    CHECK_FALSE(s3.expired());

    obj1.reset();
    CHECK(s1.expired());
    CHECK(s3.expired());

    CHECK_FALSE(s2.expired());

    obj2.reset();
    s4.track(s2);
    CHECK(s4.expired());
}

TEST_CASE("Slot::slot_function()", "[slot]")
{
    Slot<int(char, double)> s{[](char, double) { return 5; }};

    CHECK(5 == s.slot_function()('g', 3.7));

    s.slot_function() = [](char, double) { return 7; };

    CHECK(7 == s('f', 2.8));

    // void return type
    Slot<void(char, double)> s_v{[](char, double) { return 5; }};
    s_v.slot_function()('h', 7.3);
}

TEST_CASE("Slot::slot_function() const", "[slot]")
{
    const Slot<int(char, double)> s{[](char, double) { return 5; }};

    CHECK(5 == s.slot_function()('g', 3.7));
}

// Update
TEST_CASE("Slot(Signal)", "[slot]")
{
    Signal<int(int)> sig;
    Slot<std::optional<int>(int)> a_slot{sig};
}

TEST_CASE("Slot(Slot const&)", "[slot]")
{
    Slot<double(int)> s{[](int) { return 7.3; }};
    auto obj = std::make_shared<char>('u');
    s.track(obj);
    Slot<double(int)> s_copy{s};

    CHECK(7.3 == Approx(s_copy(9)).margin(1e-12));

    CHECK_FALSE(s.expired());
    CHECK_FALSE(s_copy.expired());

    obj.reset();

    CHECK(s.expired());
    CHECK(s.expired());
}

TEST_CASE("Slot(std::bind())", "[slot]")
{
    auto lmda = [](int i, double d, char c) { return i + d; };

    Slot<double(char)> s{std::bind(lmda, 5, 2.3, std::placeholders::_1)};

    CHECK(7.3 == s('k'));
    // s(2.3, 'h');
}

TEST_CASE("Slot::operator()", "[slot]")
{
    // result returned
    Slot<int(int, int)> s{[](int i, int ii) { return i + ii; }};

    auto obj = std::make_shared<double>(8.3);
    s.track(obj);

    CHECK(7 == s(4, 3));

    // objects locked - obj is not deleted in separate thread
    // exception thrown
    obj.reset();
    CHECK_THROWS_AS(s.call(4, 5), Expired_slot);

    // void return type
    Slot<void(char, double)> s_v{[](char, double) { return 5; }};
    s_v('h', 7.3);

    const Slot<int(int, int)> s2{[](int i, int ii) { return i + ii; }};
    CHECK(7 == s2(4, 3));
}

#include <memory>

#include <signals/connection.hpp>
#include <signals/detail/connection_impl.hpp>
#include <signals/slot.hpp>

#include <catch2/catch.hpp>

using sig::Connection;
using sig::Connection_impl;
using sig::Slot;

TEST_CASE("Connection_impl()", "[connection_impl]")
{
    Connection_impl<void(int)> impl;
    CHECK_FALSE(impl.connected());
    CHECK_FALSE(impl.blocked());
}

TEST_CASE("Connection_impl(Slot)", "[connection_impl]")
{
    Slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl(s);
    CHECK(impl.connected());
    CHECK_FALSE(impl.blocked());
}

TEST_CASE("Connection_impl::disconnect()", "[connection_impl]")
{
    Slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl(s);
    CHECK(impl.connected());
    CHECK_FALSE(impl.blocked());

    impl.disconnect();
    CHECK_FALSE(impl.connected());
    CHECK_FALSE(impl.blocked());
}

TEST_CASE("Connection_impl::blocked()", "[connection_impl]")
{
    Connection_impl<void(int)> impl1;
    CHECK_FALSE(impl1.blocked());

    Slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl2(s);
    CHECK_FALSE(impl2.blocked());
}

TEST_CASE("Connection_impl::connected()", "[connection_impl]")
{
    Slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl1(s);
    CHECK(impl1.connected());

    Connection_impl<void(int)> impl2;
    CHECK_FALSE(impl2.connected());
}

TEST_CASE("Connection_impl::get_slot()", "[connection_impl]")
{
    Slot<int(int)> s1 = [](int) { return 5; };
    Connection_impl<int(int)> impl(s1);

    CHECK((impl.get_slot())(1) == 5);

    Slot<int(int)> s2 = [](int) { return 7; };
    impl.get_slot()   = s2;

    CHECK((impl.get_slot())(1) == 7);
}

TEST_CASE("Connection_impl::get_slot() const", "[connection_impl]")
{
    Slot<int(int)> s1 = [](int) { return 5; };
    const Connection_impl<int(int)> impl(s1);

    CHECK((impl.get_slot())(1) == 5);
}

TEST_CASE("Connection_impl::emplace_extended()", "[connection_impl]")
{
    auto ci = std::make_shared<Connection_impl<int(double)>>();
    Slot<int(const Connection&, double)> es = [](const Connection&, double) {
        return 3;
    };
    Connection conn{ci};
    ci->emplace_extended(es, conn);
    CHECK(conn.connected());
    CHECK(ci->get_slot()(5.4) == 3);
}

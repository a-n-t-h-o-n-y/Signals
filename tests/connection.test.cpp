#include <memory>

#include <signals/connection.hpp>
#include <signals/detail/connection_impl.hpp>
#include <signals/slot.hpp>

#include <catch2/catch.hpp>

using sig::Connection;
using sig::Connection_impl;
using sig::Connection_impl_base;
using sig::Slot;

TEST_CASE("Connection()", "[connection]")
{
    Connection c;
    CHECK_FALSE(c.connected());
    CHECK_FALSE(c.blocked());

    CHECK_FALSE(c.connected());
    CHECK_FALSE(c.blocked());
}

TEST_CASE("Connection(Connection const&)", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Connection conn_2{conn};

    CHECK(conn.connected());
    CHECK(conn_2.connected());

    conn.disconnect();

    CHECK_FALSE(conn_2.connected());
    CHECK_FALSE(conn.connected());
}

TEST_CASE("Connection(Connection&&)", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Connection conn_2{std::move(conn)};

    CHECK_FALSE(conn.connected());
    CHECK(conn_2.connected());

    conn_2.disconnect();

    CHECK_FALSE(conn_2.connected());
    CHECK_FALSE(conn.connected());
}

TEST_CASE("Connection::operator=(Connection const&)", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(int, int)>>(s2);
    Connection conn2{c_impl2};

    CHECK(conn.connected());
    CHECK(conn2.connected());

    conn2.disconnect();

    CHECK(conn.connected());
    CHECK_FALSE(conn2.connected());

    conn2 = conn;

    CHECK(conn.connected());
    CHECK(conn2.connected());

    conn2 = conn2;

    CHECK(conn.connected());
    CHECK(conn2.connected());
}

TEST_CASE("Connection::operator=(Connection&&)", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(int, int)>>(s2);
    Connection conn2{c_impl2};

    CHECK(conn.connected());
    CHECK(conn2.connected());

    conn2.disconnect();

    CHECK(conn.connected());
    CHECK_FALSE(conn2.connected());

    conn2 = std::move(conn);

    CHECK_FALSE(conn.connected());
    CHECK(conn2.connected());

    conn2 = conn2;

    CHECK_FALSE(conn.connected());
    CHECK(conn2.connected());
}

TEST_CASE("Connection::disconnect()", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection my_conn{c_impl};

    CHECK(my_conn.connected());

    my_conn.disconnect();

    CHECK_FALSE(my_conn.connected());

    Slot<int(double)> s2{[](double) { return 3; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(double)>>(s2);
    Connection my_conn2{c_impl2};

    CHECK(my_conn2.connected());

    c_impl2.reset();

    CHECK_FALSE(my_conn2.connected());

    my_conn2.disconnect();

    CHECK_FALSE(my_conn2.connected());
}

TEST_CASE("Connection::connected()", "[connection]")
{
    // test null constructed Connection
    Connection c1;

    CHECK_FALSE(c1.connected());

    // test properly built Connection
    Slot<int(double)> s{[](double) { return 4; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(
        s);  // this was expiring and making Connection not connected anymore
    Connection c2{c_impl};

    CHECK(c2.connected());

    // reset the c_impl object to kill the Connection and check for a Connection
    c_impl.reset();
    CHECK_FALSE(c2.connected());
}

TEST_CASE("Connection::blocked()", "[connection]")
{
    // test null constructed Connection
    Connection c1;

    CHECK_FALSE(c1.blocked());

    // test properly build Connection
    Slot<int(double)> s{[](double) { return 4; }};
    Connection c2{std::make_shared<Connection_impl<int(double)>>(s)};

    CHECK_FALSE(c2.blocked());

    // test after shared_Connection_block applies
}

TEST_CASE("Connection::operator==", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(int, int)>>(s2);
    Connection conn2{c_impl2};

    Slot<int(int, int)> s3{[](int, int) { return 3; }};
    auto c_impl3 = std::make_shared<Connection_impl<int(int, int)>>(s3);
    Connection conn3{c_impl3};

    Connection c_null_1;
    Connection c_null_2;

    CHECK_FALSE(conn == conn2);
    CHECK(conn == conn);
    CHECK_FALSE(conn2 == conn3);

    CHECK(c_null_1 == c_null_2);
    CHECK(c_null_1 == c_null_2);

    CHECK_FALSE(c_null_1 == conn);
}

TEST_CASE("Connection::operator!=", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(int, int)>>(s2);
    Connection conn2{c_impl2};

    Slot<int(int, int)> s3{[](int, int) { return 3; }};
    auto c_impl3 = std::make_shared<Connection_impl<int(int, int)>>(s3);
    Connection conn3{c_impl3};

    Connection c_null_1;
    Connection c_null_2;

    CHECK(conn != conn2);
    CHECK_FALSE(conn != conn);
    CHECK(conn2 != conn3);

    CHECK_FALSE(c_null_1 != c_null_2);
    CHECK_FALSE(c_null_1 != c_null_2);

    CHECK(c_null_1 != conn);
}

TEST_CASE("Connection::operator<", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(int, int)>>(s2);
    Connection conn2{c_impl2};

    Slot<int(int, int)> s3{[](int, int) { return 3; }};
    auto c_impl3 = std::make_shared<Connection_impl<int(int, int)>>(s3);
    Connection conn3{c_impl3};

    CHECK_FALSE(conn < conn);

    typedef Connection_impl_base base;

    if (std::dynamic_pointer_cast<base>(c_impl) <
        std::dynamic_pointer_cast<base>(c_impl2)) {
        CHECK(conn < conn2);
        CHECK_FALSE(conn2 < conn);
    }

    if (std::dynamic_pointer_cast<base>(c_impl2) <
        std::dynamic_pointer_cast<base>(c_impl)) {
        CHECK(conn2 < conn);
        CHECK_FALSE(conn < conn2);
    }

    if (std::dynamic_pointer_cast<base>(c_impl3) <
        std::dynamic_pointer_cast<base>(c_impl2)) {
        CHECK(conn3 < conn2);
        CHECK_FALSE(conn2 < conn3);
    }

    if (std::dynamic_pointer_cast<base>(c_impl2) <
        std::dynamic_pointer_cast<base>(c_impl3)) {
        CHECK(conn2 < conn3);
        CHECK_FALSE(conn3 < conn2);
    }

    if (std::dynamic_pointer_cast<base>(c_impl3) <
        std::dynamic_pointer_cast<base>(c_impl)) {
        CHECK(conn3 < conn);
        CHECK_FALSE(conn < conn3);
    }

    if (std::dynamic_pointer_cast<base>(c_impl) <
        std::dynamic_pointer_cast<base>(c_impl3)) {
        CHECK(conn < conn3);
        CHECK_FALSE(conn3 < conn);
    }
}

TEST_CASE("Connection::swap()", "[connection]")
{
    Slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<Connection_impl<int(double)>>(s);
    Connection conn{c_impl};

    Slot<int(int, int)> s2{[](int, int) { return 7; }};
    auto c_impl2 = std::make_shared<Connection_impl<int(int, int)>>(s2);
    Connection conn2{c_impl2};

    CHECK(conn.connected());
    CHECK(conn2.connected());

    conn.disconnect();

    CHECK_FALSE(conn.connected());
    CHECK(conn2.connected());

    using std::swap;
    swap(conn2, conn);

    CHECK(conn.connected());
    CHECK_FALSE(conn2.connected());
}

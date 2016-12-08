#include <connection.hpp>
#include <detail/connection_impl.hpp>
#include <slot.hpp>

#include <gtest/gtest.h>

#include <memory>

// just makes sure the pointer to base class works by calling the
// implementation functons

TEST(ConnectionTest, DefaultConstructor) {
    mcurses::Connection c;
    EXPECT_FALSE(c.connected());
    EXPECT_FALSE(c.blocked());

    EXPECT_FALSE(c.connected());
    EXPECT_FALSE(c.blocked());
}

TEST(ConnectionTest, CopyConstructor) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::Connection conn_2{conn};

    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn_2.connected());

    conn.disconnect();

    EXPECT_FALSE(conn_2.connected());
    EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, MoveConstructor) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::Connection conn_2{std::move(conn)};

    EXPECT_FALSE(conn.connected());
    EXPECT_TRUE(conn_2.connected());

    conn_2.disconnect();

    EXPECT_FALSE(conn_2.connected());
    EXPECT_FALSE(conn.connected());
}

TEST(ConnectionTest, CopyAssignmentOperator) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s2);
    mcurses::Connection conn2{c_impl2};

    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn2.connected());

    conn2.disconnect();

    EXPECT_TRUE(conn.connected());
    EXPECT_FALSE(conn2.connected());

    conn2 = conn;

    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn2.connected());

    conn2 = conn2;

    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn2.connected());
}

TEST(ConnectionTest, MoveAssignmentOperator) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s2);
    mcurses::Connection conn2{c_impl2};

    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn2.connected());

    conn2.disconnect();

    EXPECT_TRUE(conn.connected());
    EXPECT_FALSE(conn2.connected());

    conn2 = std::move(conn);

    EXPECT_FALSE(conn.connected());
    EXPECT_TRUE(conn2.connected());

    conn2 = conn2;

    EXPECT_FALSE(conn.connected());
    EXPECT_TRUE(conn2.connected());
}

TEST(ConnectionTest, DisconnectMethod) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection my_conn{c_impl};

    EXPECT_TRUE(my_conn.connected());

    my_conn.disconnect();

    EXPECT_FALSE(my_conn.connected());

    mcurses::slot<int(double)> s2{[](double) { return 3; }};
    auto c_impl2 = std::make_shared<mcurses::Connection_impl<int(double)>>(s2);
    mcurses::Connection my_conn2{c_impl2};

    EXPECT_TRUE(my_conn2.connected());

    c_impl2.reset();

    EXPECT_FALSE(my_conn2.connected());

    my_conn2.disconnect();

    EXPECT_FALSE(my_conn2.connected());
}

TEST(ConnectionTest, ConnectedMethod) {
    // test null constructed Connection
    mcurses::Connection c1;

    EXPECT_FALSE(c1.connected());

    // test properly built Connection
    mcurses::slot<int(double)> s{[](double) { return 4; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(
        s);  // this was expiring and making Connection not connected anymore
    mcurses::Connection c2{c_impl};

    EXPECT_TRUE(c2.connected());

    // reset the c_impl object to kill the Connection and check for a Connection
    c_impl.reset();
    EXPECT_FALSE(c2.connected());
}

TEST(ConnectionTest, BlockedMethod) {
    // test null constructed Connection
    mcurses::Connection c1;

    EXPECT_FALSE(c1.blocked());

    // test properly build Connection
    mcurses::slot<int(double)> s{[](double) { return 4; }};
    mcurses::Connection c2{
        std::make_shared<mcurses::Connection_impl<int(double)>>(s)};

    EXPECT_FALSE(c2.blocked());

    // test after shared_Connection_block applies
}

TEST(ConnectionTest, OperatorEquals) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s2);
    mcurses::Connection conn2{c_impl2};

    mcurses::slot<int(int, int)> s3{[](int, int) { return 3; }};
    auto c_impl3 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s3);
    mcurses::Connection conn3{c_impl3};

    mcurses::Connection c_null_1;
    mcurses::Connection c_null_2;

    EXPECT_FALSE(conn == conn2);
    EXPECT_TRUE(conn == conn);
    EXPECT_FALSE(conn2 == conn3);

    EXPECT_TRUE(c_null_1 == c_null_2);
    EXPECT_TRUE(c_null_1 == c_null_2);

    EXPECT_FALSE(c_null_1 == conn);
}

TEST(ConnectionTest, OperatorNotEquals) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s2);
    mcurses::Connection conn2{c_impl2};

    mcurses::slot<int(int, int)> s3{[](int, int) { return 3; }};
    auto c_impl3 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s3);
    mcurses::Connection conn3{c_impl3};

    mcurses::Connection c_null_1;
    mcurses::Connection c_null_2;

    EXPECT_TRUE(conn != conn2);
    EXPECT_FALSE(conn != conn);
    EXPECT_TRUE(conn2 != conn3);

    EXPECT_FALSE(c_null_1 != c_null_2);
    EXPECT_FALSE(c_null_1 != c_null_2);

    EXPECT_TRUE(c_null_1 != conn);
}

TEST(ConnectionTest, OperatorLessThan) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::slot<int(int, int)> s2{[](int, int) { return 3; }};
    auto c_impl2 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s2);
    mcurses::Connection conn2{c_impl2};

    mcurses::slot<int(int, int)> s3{[](int, int) { return 3; }};
    auto c_impl3 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s3);
    mcurses::Connection conn3{c_impl3};

    EXPECT_FALSE(conn < conn);

    typedef mcurses::Connection_impl_base base;

    if (std::dynamic_pointer_cast<base>(c_impl) <
        std::dynamic_pointer_cast<base>(c_impl2)) {
        EXPECT_TRUE(conn < conn2);
        EXPECT_FALSE(conn2 < conn);
    }

    if (std::dynamic_pointer_cast<base>(c_impl2) <
        std::dynamic_pointer_cast<base>(c_impl)) {
        EXPECT_TRUE(conn2 < conn);
        EXPECT_FALSE(conn < conn2);
    }

    if (std::dynamic_pointer_cast<base>(c_impl3) <
        std::dynamic_pointer_cast<base>(c_impl2)) {
        EXPECT_TRUE(conn3 < conn2);
        EXPECT_FALSE(conn2 < conn3);
    }

    if (std::dynamic_pointer_cast<base>(c_impl2) <
        std::dynamic_pointer_cast<base>(c_impl3)) {
        EXPECT_TRUE(conn2 < conn3);
        EXPECT_FALSE(conn3 < conn2);
    }

    if (std::dynamic_pointer_cast<base>(c_impl3) <
        std::dynamic_pointer_cast<base>(c_impl)) {
        EXPECT_TRUE(conn3 < conn);
        EXPECT_FALSE(conn < conn3);
    }

    if (std::dynamic_pointer_cast<base>(c_impl) <
        std::dynamic_pointer_cast<base>(c_impl3)) {
        EXPECT_TRUE(conn < conn3);
        EXPECT_FALSE(conn3 < conn);
    }
}

TEST(ConnectionTest, Swap) {
    mcurses::slot<int(double)> s{[](double) { return 3; }};
    auto c_impl = std::make_shared<mcurses::Connection_impl<int(double)>>(s);
    mcurses::Connection conn{c_impl};

    mcurses::slot<int(int, int)> s2{[](int, int) { return 7; }};
    auto c_impl2 =
        std::make_shared<mcurses::Connection_impl<int(int, int)>>(s2);
    mcurses::Connection conn2{c_impl2};

    EXPECT_TRUE(conn.connected());
    EXPECT_TRUE(conn2.connected());

    conn.disconnect();

    EXPECT_FALSE(conn.connected());
    EXPECT_TRUE(conn2.connected());

    using std::swap;
    swap(conn2, conn);

    EXPECT_TRUE(conn.connected());
    EXPECT_FALSE(conn2.connected());
}

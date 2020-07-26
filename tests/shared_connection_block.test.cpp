#include <memory>

#include <signals/connection.hpp>
#include <signals/shared_connection_block.hpp>
#include <signals/signal.hpp>
#include <signals/slot.hpp>

#include <catch2/catch.hpp>

using sig::Connection;
using sig::Shared_connection_block;
using sig::Signal;
using sig::Slot;

TEST_CASE("Shared_connection_block()", "[shared_connection_block]")
{
    Shared_connection_block scb1{};
    CHECK_FALSE(scb1.blocking());

    Signal<int(char)> sig1;
    Slot<int(char)> slot_1 = [](char) { return 3; };
    auto conn              = sig1.connect(slot_1);

    CHECK_FALSE(conn.blocked());

    Shared_connection_block scb2(conn);

    CHECK(conn.blocked());
    CHECK(scb2.blocking());
    CHECK_FALSE(bool(sig1('g')));

    Signal<int(char)> sig2;
    Slot<int(char)> slot_2 = [](char) { return 3; };
    auto conn2             = sig2.connect(slot_2);

    CHECK_FALSE(conn2.blocked());

    Shared_connection_block scb3(conn2, false);
    CHECK_FALSE(conn2.blocked());
    CHECK_FALSE(scb3.blocking());

    scb3.block();
    CHECK(conn2.blocked());
    CHECK(scb3.blocking());
}

TEST_CASE("Shared_connection_block(Connection)", "[shared_connection_block]")
{
    Signal<int()> sig;
    Slot<int()> slot_1 = []() { return 2; };
    auto conn1         = sig.connect(slot_1);

    Shared_connection_block scb2{conn1};

    CHECK(conn1.connected());
    scb2.connection().disconnect();
    CHECK_FALSE(conn1.connected());

    Shared_connection_block scb_empty;
    CHECK_FALSE(scb_empty.connection().connected());
}

TEST_CASE("Shared_connection_block(Shared_connection_block const&)",
          "[shared_connection_block]")
{
    Signal<int(char)> sig1;
    Slot<int(char)> slot_1 = [](char) { return 3; };
    auto conn1             = sig1.connect(slot_1);
    Shared_connection_block scb1(conn1);

    CHECK(scb1.blocking());

    Shared_connection_block scb1_copy(scb1);

    CHECK(scb1_copy.blocking());
    CHECK(conn1.blocked());

    scb1.unblock();
    CHECK_FALSE(scb1.blocking());
    CHECK(scb1_copy.blocking());
    CHECK(conn1.blocked());

    scb1_copy.block();
    CHECK(scb1_copy.blocking());
    CHECK_FALSE(scb1.blocking());
    CHECK(conn1.blocked());

    scb1_copy.unblock();
    CHECK_FALSE(scb1.blocking());
    CHECK_FALSE(conn1.blocked());
}

TEST_CASE("Shared_connection_block::operator=(Shared_connection_block const&)",
          "[shared_connection_block]")
{
    Signal<int()> sig1;
    Slot<int()> slot_1 = []() { return 4; };
    auto conn1         = sig1.connect(slot_1);

    Shared_connection_block scb1{conn1};
    Shared_connection_block scb2;
    scb2 = scb1;

    CHECK(conn1.blocked());
    CHECK(scb2.blocking());

    scb1.unblock();

    CHECK(scb2.blocking());
    CHECK_FALSE(scb1.blocking());
    CHECK(conn1.blocked());

    scb2.unblock();

    CHECK_FALSE(scb1.blocking());
    CHECK_FALSE(scb2.blocking());
    CHECK_FALSE(conn1.blocked());

    scb1.block();

    CHECK(scb1.blocking());
    CHECK_FALSE(scb2.blocking());
    CHECK(conn1.blocked());

    Signal<int(double)> sig2;
    Slot<int(double)> slot_2                      = [](double) { return 4; };
    auto conn2                                    = sig2.connect(slot_2);
    Slot<int(const Connection&, double)> slot_ext = [](const Connection&,
                                                       double) { return 5; };
    auto conn3 = sig2.connect_extended(1, slot_ext);

    Shared_connection_block scb4{conn2};
    Shared_connection_block scb3{conn3};

    CHECK(conn2.blocked());
    CHECK(conn3.blocked());
    CHECK_FALSE(bool(sig2(5.4)));

    scb3 = scb4;

    CHECK(conn2.blocked());
    CHECK_FALSE(conn3.blocked());
    CHECK(scb4.blocking());
    CHECK(scb3.blocking());
    REQUIRE(bool(sig2(5.4)));
    CHECK(*sig2(5.4) == 5);

    scb4.unblock();
    CHECK(conn2.blocked());
    CHECK_FALSE(conn3.blocked());
    CHECK_FALSE(scb4.blocking());
    CHECK(scb3.blocking());
    REQUIRE(bool(sig2(5.4)));
    CHECK(*sig2(5.4) == 5);

    scb3.unblock();
    CHECK_FALSE(conn2.blocked());
    CHECK_FALSE(conn3.blocked());
    CHECK_FALSE(scb4.blocking());
    CHECK_FALSE(scb3.blocking());
    REQUIRE(bool(sig2(5.4)));
    CHECK(*sig2(5.4) == 4);
}

TEST_CASE("~Shared_connection_block()", "[shared_connection_block]")
{
    Signal<int(double)> sig1;
    Slot<int(double)> slot_1;
    auto conn1 = sig1.connect(slot_1);

    auto scb_ptr = std::make_shared<Shared_connection_block>(conn1);

    CHECK(scb_ptr->blocking());
    CHECK(conn1.blocked());

    scb_ptr.reset();

    CHECK_FALSE(conn1.blocked());
}

TEST_CASE("Shared_connection_block::block()", "[shared_connection_block]")
{
    Signal<int()> sig;
    Slot<int()> slot_1;
    auto conn1 = sig.connect(slot_1);

    Shared_connection_block scb{conn1, false};
    CHECK_FALSE(conn1.blocked());
    CHECK_FALSE(scb.blocking());

    scb.block();
    CHECK(conn1.blocked());
    CHECK(scb.blocking());

    scb.block();
    CHECK(conn1.blocked());
    CHECK(scb.blocking());

    scb.block();
    scb.block();
    scb.block();
    CHECK(conn1.blocked());
    CHECK(scb.blocking());

    scb.unblock();
    CHECK_FALSE(conn1.blocked());
    CHECK_FALSE(scb.blocking());

    Shared_connection_block scb2{conn1};
    scb.block();
    CHECK(conn1.blocked());
    CHECK(scb.blocking());
    CHECK(scb2.blocking());
    scb.block();
    scb2.block();
    CHECK(conn1.blocked());
    CHECK(scb.blocking());
    CHECK(scb2.blocking());
    scb.unblock();
    CHECK(conn1.blocked());
    CHECK_FALSE(scb.blocking());
    CHECK(scb2.blocking());
    scb2.unblock();
    CHECK_FALSE(conn1.blocked());
    CHECK_FALSE(scb.blocking());
    CHECK_FALSE(scb2.blocking());
}

TEST_CASE("Shared_connection_block::unblock()", "[shared_connection_block]")
{
    Signal<void(int)> sig;
    Slot<void(int)> slot_1 = [](int) { return; };
    auto conn1             = sig.connect(slot_1);

    Shared_connection_block scb{conn1, true};
    CHECK(conn1.blocked());
    CHECK(scb.blocking());

    scb.unblock();
    scb.unblock();
    scb.unblock();
    scb.unblock();
    scb.unblock();
    scb.unblock();

    CHECK_FALSE(conn1.blocked());
    CHECK_FALSE(scb.blocking());
}

TEST_CASE("Shared_connection_block::blocking()", "[shared_connection_block]")
{
    Shared_connection_block scb1;
    CHECK_FALSE(scb1.blocking());

    Signal<int()> sig;
    Slot<int()> slot_1 = []() { return 2; };
    auto conn1         = sig.connect(slot_1);

    Shared_connection_block scb2{conn1};
    Shared_connection_block scb3{conn1, false};

    CHECK(scb2.blocking());
    CHECK_FALSE(scb3.blocking());

    scb3.block();
    CHECK(scb3.blocking());

    scb2.unblock();
    CHECK_FALSE(scb2.blocking());
}

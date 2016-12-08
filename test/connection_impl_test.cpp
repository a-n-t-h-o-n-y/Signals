#include <detail/connection_impl.hpp>
#include <slot.hpp>

#include <gtest/gtest.h>

using mcurses::Connection_impl;
using mcurses::slot;

TEST(ConnectionImplTest, DefaultConstructor) {
    Connection_impl<void(int)> impl;
    EXPECT_FALSE(impl.connected());
    EXPECT_FALSE(impl.blocked());
}

TEST(ConnectionImplTest, SlotConstructor) {
    slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl(s);
    EXPECT_TRUE(impl.connected());
    EXPECT_FALSE(impl.blocked());
}

TEST(ConnectionImplTest, DisconnectMethod) {
    slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl(s);
    EXPECT_TRUE(impl.connected());
    EXPECT_FALSE(impl.blocked());

    impl.disconnect();
    EXPECT_FALSE(impl.connected());
    EXPECT_FALSE(impl.blocked());
}

TEST(ConnectionImplTest, BlockedMethod) {
    Connection_impl<void(int)> impl1;
    EXPECT_FALSE(impl1.blocked());

    slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl2(s);
    EXPECT_FALSE(impl2.blocked());

    // Fill in once you have shared_connection_block
}

TEST(ConnectionImplTest, ConnectedMethod) {
    slot<void(int)> s = [](int) { return; };
    Connection_impl<void(int)> impl1(s);
    EXPECT_TRUE(impl1.connected());

    Connection_impl<void(int)> impl2;
    EXPECT_FALSE(impl2.connected());
}

TEST(ConnectionImplTest, GetSlotMethod) {
    slot<int(int)> s1 = [](int) { return 5; };
    Connection_impl<int(int)> impl(s1);

    EXPECT_EQ(5, (impl.get_slot())(1));

    slot<int(int)> s2 = [](int) { return 7; };
    impl.get_slot() = s2;

    EXPECT_EQ(7, (impl.get_slot())(1));
}

TEST(ConnectionImplTest, ConstGetSlotMethod) {
    slot<int(int)> s1 = [](int) { return 5; };
    const Connection_impl<int(int)> impl(s1);

    EXPECT_EQ(5, (impl.get_slot())(1));
}

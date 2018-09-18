# FIND GTEST
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
find_package(GTest)
add_library(gtest INTERFACE IMPORTED)
set_property(TARGET gtest PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS})
set_property(TARGET gtest PROPERTY
    INTERFACE_LINK_LIBRARIES ${GTEST_BOTH_LIBRARIES})

# CREATE TESTS
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
add_executable(test_signals EXCLUDE_FROM_ALL
    slot_test.cpp
    slot_base_test.cpp
    connection_test.cpp
    connection_impl_test.cpp
    signal_test.cpp
    signal_impl_test.cpp
    shared_connection_block_test.cpp
    optional_last_value_test.cpp
)

target_link_libraries(test_signals PRIVATE gtest signals)
target_compile_features(test_signals PRIVATE cxx_std_14)

add_custom_target(tests DEPENDS test_signals)

add_test(test_signals test_signals)

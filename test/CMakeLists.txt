# FIND GTEST
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
find_package(GTest)
find_package(Threads)
add_library(gtest INTERFACE IMPORTED)
set_property(TARGET gtest PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${GTEST_INCLUDE_DIRS})
set_property(TARGET gtest PROPERTY
    INTERFACE_LINK_LIBRARIES ${GTEST_BOTH_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

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

if(${CMAKE_MAJOR_VERSION} VERSION_LESS "3.8")
    set(CMAKE_CXX_STANDARD 14)
else()
    target_compile_features(test_signals INTERFACE cxx_std_14)
endif()

add_custom_target(tests DEPENDS test_signals)

add_test(test_signals test_signals)

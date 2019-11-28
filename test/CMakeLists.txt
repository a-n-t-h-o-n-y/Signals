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
add_executable(signals_test EXCLUDE_FROM_ALL
    slot_test.cpp
    slot_base_test.cpp
    connection_test.cpp
    connection_impl_test.cpp
    signal_test.cpp
    signal_impl_test.cpp
    shared_connection_block_test.cpp
    optional_last_value_test.cpp
)

target_link_libraries(signals_test PUBLIC gtest signals)

if(${CMAKE_VERSION} VERSION_LESS "3.8")
    set(CMAKE_CXX_STANDARD 14)
else()
    target_compile_features(signals_test INTERFACE cxx_std_14)
endif()

add_test(signals_test signals_test)

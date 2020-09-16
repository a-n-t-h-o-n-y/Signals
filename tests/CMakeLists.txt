cmake_minimum_required(VERSION 3.5.1)

add_executable(signals_test EXCLUDE_FROM_ALL
    connection.test.cpp
    connection_impl.test.cpp
    optional_last_value.test.cpp
    shared_connection_block.test.cpp
    signal.test.cpp
    slot.test.cpp
    slot_base.test.cpp
)

target_link_libraries(signals_test PUBLIC signals catch_two)

if(${CMAKE_VERSION} VERSION_LESS "3.8")
    set(CMAKE_CXX_STANDARD 17)
else()
    target_compile_features(signals_test INTERFACE cxx_std_17)
endif()

add_test(signals_test signals_test)

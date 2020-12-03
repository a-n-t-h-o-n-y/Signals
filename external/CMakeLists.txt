cmake_minimum_required(VERSION 3.5.1)

if (NOT TARGET Catch2)
    add_subdirectory(catch-2)
endif()

# Catch2 Main
add_library(catch_two EXCLUDE_FROM_ALL catch2.main.cpp)
target_link_libraries(catch_two PUBLIC Catch2::Catch2)

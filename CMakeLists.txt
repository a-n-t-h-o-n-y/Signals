cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(libsignals LANGUAGES CXX)
include(GNUInstallDirs)
enable_testing()

# Create Signals Library
add_library(signals INTERFACE)

target_include_directories(signals
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(${CMAKE_VERSION} VERSION_LESS "3.8")
    set(CMAKE_CXX_STANDARD 17)
else()
    target_compile_features(signals INTERFACE cxx_std_17)
endif()

# Install Signals Library
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS signals EXPORT SignalsLibraryConfig)
install(EXPORT SignalsLibraryConfig DESTINATION share/SignalsLibrary/cmake)
export(
    TARGETS
        signals
    FILE
        SignalsLibraryConfig.cmake
)

# Tests
add_subdirectory(external/)
add_subdirectory(tests/)

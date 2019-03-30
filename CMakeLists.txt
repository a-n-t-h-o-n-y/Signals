cmake_minimum_required(VERSION 3.2 FATAL_ERROR)
project(libsignals LANGUAGES CXX)
include(GNUInstallDirs)
enable_testing()

add_subdirectory(libs/Optional)

# CREATE SIGNALS LIBRARY
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
add_library(signals INTERFACE)

target_include_directories(signals
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

if(${CMAKE_VERSION} VERSION_LESS "3.8")
    set(CMAKE_CXX_STANDARD 14)
else()
    target_compile_features(signals INTERFACE cxx_std_14)
endif()

target_link_libraries(signals INTERFACE optional)

# LIBRARY INSTALLATION
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(TARGETS signals EXPORT SignalsLibraryConfig)
install(EXPORT SignalsLibraryConfig DESTINATION share/SignalsLibrary/cmake)
export(
    TARGETS
        signals
        optional
    FILE
        SignalsLibraryConfig.cmake
)

# ADD TESTS
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
add_subdirectory(test)

# DOXYGEN
# - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target(signals_docs
        ${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/docs/Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/docs
        COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif()

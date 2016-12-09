cmake_minimum_required (VERSION 3.6)
project (Signals)

# Uncomment the next three lines to compile with clang static analyzer
# set(CCC_CXX clang++)
# set(CMAKE_CXX_COMPILER /usr/lib/clang/c++-analyzer)
# set(CMAKE_LINKER clang++)

# Build System Information For Clang-Tidy
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_custom_target(compile_commands ALL
	COMMAND [ -f ${CMAKE_SOURCE_DIR}/build/compile_commands.json ] && mv ${CMAKE_SOURCE_DIR}/build/compile_commands.json
		${CMAKE_SOURCE_DIR}/compile_commands.json || :)

# Set Output Directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/lib")

# Set Compiler Flags
set(CMAKE_CXX_FLAGS "-std=c++14 -g -Wall")

# Include Directory
include_directories(include)

# Collect Source Files into Variable Names
list(APPEND SIGNAL_MODULE_SOURCES 	"src/slot_base.cpp"
									)

list(APPEND TEST_SOURCES "test/slot_test.cpp"
						 "test/slot_base_test.cpp"
						 "test/connection_test.cpp"
						 "test/connection_impl_test.cpp"
						 "test/signal_test.cpp"
						 "test/signal_impl_test.cpp"
						 "test/shared_connection_block_test.cpp"
                         "test/optional_last_value_test.cpp"
						 )

# Create Static Library
add_library(signals STATIC ${SIGNAL_MODULE_SOURCES})

# Create Executables with the Sources
add_executable(tests ${TEST_SOURCES})

# Link libraries to executables
target_link_libraries(tests signals gtest gtest_main)

# Install Static Library
install(TARGETS signals ARCHIVE DESTINATION lib)

# Install Headers
install(DIRECTORY include/ DESTINATION include/aml/signals)

# Doxygen Target - Doxyfile in project/doc - make doc
find_package(Doxygen)
if(DOXYGEN_FOUND)
add_custom_target(doc
${DOXYGEN_EXECUTABLE} ${CMAKE_SOURCE_DIR}/doc/Doxyfile
WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doc
COMMENT "Generating API documentation with Doxygen" VERBATIM)
endif(DOXYGEN_FOUND)

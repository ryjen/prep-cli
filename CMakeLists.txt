#latest cmake for C++11 flag
cmake_minimum_required (VERSION 3.1)

#use c++17
set(CMAKE_CXX_STANDARD 17)

# add options for testing
option(CODE_COVERAGE "Enable code coverage testing." OFF)
option(MEMORY_CHECK "Enable testing for memory leaks." OFF)

# define project name
project (prep VERSION 0.1)

# set path to custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# create the package config
include(CreatePackage)
create_package("a c/c++ package and project manager")

# define the library name
set(PROJECT_LIBRARY "utensil")

if (APPLE)

# osx has its own libarchive but without headers
# set the pkg config path to prefer the homebrew installation
set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libarchive/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

endif()

find_package(PkgConfig REQUIRED)

# find a libarchive.  hint to a homebrew installation for the header files
pkg_search_module(LibArchive REQUIRED libarchive)

find_program(ZIP_EXECUTABLE zip)

if (NOT ZIP_EXECUTABLE)
	message(FATAL "A zip program required")
endif()

# add target for code coverage
if(CODE_COVERAGE)
	include(CodeCoverage)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
	setup_target_for_coverage(${PROJECT_NAME}-coverage ${PROJECT_SOURCE_DIR}/coverage)
endif()

# add directories
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(util)

# Setup testing
enable_testing()

if (MEMORY_CHECK)
	include(ValgrindTest)
	add_valgrind_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
else ()
	add_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
endif()

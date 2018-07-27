#latest cmake for C++11 flag
cmake_minimum_required (VERSION 3.1)

#use c++17
set(CMAKE_CXX_STANDARD 17)

# add options for testing
option(CODE_COVERAGE "Enable code coverage testing." OFF)
option(MEMORY_CHECK "Enable testing for memory leaks." OFF)

# define project name
project (prep VERSION 0.1.0)

# set path to custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# create the package config
include(CreatePackage)
create_package(DESCRIPTION "a c/c++ package and project manager")

set(PROJECT_LIBRARY "utensil")

if (APPLE)

# osx has its own libarchive but without headers
# set the pkg config path to prefer the homebrew installation
set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libarchive/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

endif()

find_package(PkgConfig REQUIRED)

# find a libarchive.
pkg_search_module(LibArchive REQUIRED libarchive)

find_library(LIB_UTIL util)

# add target for code coverage
if(CODE_COVERAGE)
	include(CodeCoverage)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
	setup_target_for_coverage(${PROJECT_NAME}-coverage ${PROJECT_SOURCE_DIR}/coverage)
endif()

# add directories
add_subdirectory(plugins)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(util)
add_subdirectory(man)

# Setup testing
enable_testing()

if (MEMORY_CHECK)
	include(ValgrindTest)
	add_valgrind_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
else ()
	add_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
endif()

# add a target to generate API documentation with Doxygen
find_package(Doxygen)
if(DOXYGEN_FOUND)
	configure_file(${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
	add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
endif(DOXYGEN_FOUND)

install(FILES FindPrep.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_VERSION}/Modules)

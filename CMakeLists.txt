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
include(CreatePackages)
create_packages(DESCRIPTION "a modular c/c++ dependency manager and build tool")

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
find_library(LIB_FTS fts)

if (NOT LIB_FTS)
  set(LIB_FTS "")
endif()

# Setup testing
enable_testing()

# add directories
add_subdirectory(plugins)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(util)
add_subdirectory(man)

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

find_path(CMAKE_PATH cmake)

install(FILES FindPrep.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_VERSION}/Modules)



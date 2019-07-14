#---------------------------------------------------------------------------------------------------------
# cmake required version
#---------------------------------------------------------------------------------------------------------

cmake_minimum_required (VERSION 3.1)

#---------------------------------------------------------------------------------------------------------
# define project
#---------------------------------------------------------------------------------------------------------

project (prep VERSION 0.1.0)

#---------------------------------------------------------------------------------------------------------
# set variables
#---------------------------------------------------------------------------------------------------------

# set path to custom cmake modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# use c++17
set(CMAKE_CXX_STANDARD 17)

# library name for this project
set(PROJECT_LIBRARY "utensil")

#---------------------------------------------------------------------------------------------------------
# create the package config for this project
#---------------------------------------------------------------------------------------------------------

include(CreatePackages)
create_packages(DESCRIPTION "a flexible and modular c/c++ dependency manager and build tool")

#---------------------------------------------------------------------------------------------------------
# Setup testing
#---------------------------------------------------------------------------------------------------------

# add options for testing
option(CODE_COVERAGE "Enable code coverage testing." OFF)
option(MEMORY_CHECK "Enable testing for memory leaks." OFF)

enable_testing()

#---------------------------------------------------------------------------------------------------------
# setup and find dependencies
#---------------------------------------------------------------------------------------------------------

if (APPLE)

  # osx has its own libarchive but without headers
  # set the pkg config path to prefer the homebrew installation
  set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libarchive/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")

endif()

find_library(LIB_UTIL util)

find_package(PkgConfig REQUIRED)

# find a libarchive.
pkg_search_module(LibArchive REQUIRED libarchive)

find_library(LIB_UTIL util)
find_library(LIB_FTS fts)

find_program(PACKAGER_PROGRAM dpkg-deb)

if (NOT LIB_FTS)
  set(LIB_FTS "")
endif()

# add directories
#---------------------------------------------------------------------------------------------------------

add_subdirectory(plugins)
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(util)
add_subdirectory(man)

#---------------------------------------------------------------------------------------------------------
# add a target to generate API documentation with Doxygen
#---------------------------------------------------------------------------------------------------------

find_package(Doxygen QUIET)

# add a target to generate API documentation with Doxygen
find_package(Doxygen)

if(DOXYGEN_FOUND)

  configure_file(${CMAKE_CURRENT_LIST_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

  add_custom_target(doc
		${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
		WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
		COMMENT "Generating API documentation with Doxygen" VERBATIM
	)
else()

  message(WARNING "install doxygen to create documentation")

endif()

if (PACKAGER_PROGRAM)

  install(DIRECTORY ${CMAKE_BINARY_DIR}/gen/plugins DESTINATION ${CMAKE_INSTALL_PREFIX}/share/prep PATTERN "main" PERMISSIONS OWNER_WRITE OWNER_READ OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

endif()

#---------------------------------------------------------------------------------------------------------
# install the project cmake module for the current cmake version
#---------------------------------------------------------------------------------------------------------

find_path(CMAKE_PATH cmake)

install(FILES FindPrep.cmake DESTINATION ${CMAKE_INSTALL_PREFIX}/share/cmake-${CMAKE_VERSION}/Modules)

#---------------------------------------------------------------------------------------------------------
# Create DEB
#---------------------------------------------------------------------------------------------------------

# Tell CPack to generate a .deb package
set(CPACK_GENERATOR "DEB")

# Set a Package Maintainer.
# This is required
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Ryan Jennings <ryan@micrantha.com>")

set(CPACK_PACKAGE_VENDOR "Micrantha Software Solutions")

# Set a Package Version
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})

set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PKGCONF_DESCRIPTION})

# Override cmake's install prefix, just in case
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr/local")

set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")

# Include CPack
include(CPack)


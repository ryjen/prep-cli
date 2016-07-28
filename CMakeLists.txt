cmake_minimum_required (VERSION 3.1)
set(CMAKE_CXX_STANDARD 11)

# add options for testing
option(CODE_COVERAGE "Enable code coverage testing." OFF)
option(MEMORY_CHECK "Enable testing for memory leaks." OFF)

# define project name
project (prep VERSION 0.1)

# set path to custom modules
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# create the package config install
include(CreatePackage)
create_package("a c/c++ package and project manager")

set(PROJECT_LIBRARY "utensil")

find_package(CURL)
find_package(ZLIB)

find_file(HAVE_ARCHIVE_H archive.h HINTS "/usr/local/opt/libarchive/include")
if (HAVE_ARCHIVE_H)
    set(LibArchive_INCLUDE_DIR "/usr/local/opt/libarchive/include")
    find_file(HAVE_ARCHIVE_ENTRY_H NAMES archive_entry.h HINTS ${LibArchive_INCLUDE_DIR})
endif()
find_library(LibArchive_LIBRARY archive HINTS "/usr/local/opt/libarchive/lib")
find_package(LibArchive)

find_library(ICONV_LIBRARIES iconv)
find_library(GIT2_LIBRARIES git2)

find_file(HAVE_LIBGEN_H NAMES libgen.h)
find_file(HAVE_CURL_CURL_H NAMES ${CURL_INCLUDE_DIRS})

# add target for code coverage
if(CODE_COVERAGE)
	include(CodeCoverage)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
	setup_target_for_coverage(${PROJECT_NAME}-coverage ${PROJECT_SOURCE_DIR}/coverage)
endif()

# create config header
include(CreateConfigHeader)
create_config_header(src/config.h.in)

# add directories
add_subdirectory(src)
add_subdirectory(tests)

# Setup testing
enable_testing()

if (MEMORY_CHECK)
	include(ValgrindTest)
	add_valgrind_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
else ()
	add_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
endif()

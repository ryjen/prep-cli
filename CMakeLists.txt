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
create_package("@PROJECT_DESCRIPTION@")

# create config header
include(CreateConfigHeader)
create_config_header(src/config.h.in)

find_package(JSONC)
find_package(CURL)

find_library(HAVE_LIBZ, z)
find_library(HAVE_LIBICONV, iconv)
find_library(HAVE_LIBGIT2, git2)
find_library(HAVE_LIBARCHIVE archive HINTS /usr/local/opt/libarchive)

find_file(HAVE_LIBGEN_H NAMES libgen.h)
find_file(HAVE_ARCHIVE_ENTRY_H NAMES archive_entry.h HINTS /usr/local/opt/libarchive)
find_file(HAVE_CURL_CURL_H NAMES curl/curl.h)
find_file(HAVE_JSON_C_H NAMES json-c.h json_c.h json.h)

# add target for code coverage
if(CODE_COVERAGE)
	include(CodeCoverage)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
	setup_target_for_coverage(${PROJECT_NAME}-coverage ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test ${PROJECT_SOURCE_DIR}/coverage)
endif()

# add directories
add_subdirectory(src)
add_subdirectory(tests)

# Setup testing
enable_testing()

if (MEMORY_CHECK)
	include(MemCheckTest)
	add_memcheck_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
else ()
	add_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
endif()

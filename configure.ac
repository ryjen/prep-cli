#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([prep], [0.1], [c0der78@gmail.com])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES(Makefile src/Makefile)
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(json-c, json_tokener_parse, [], [AC_MSG_ERROR([libjson-c is required])])
AC_CHECK_LIB(git2, git_libgit2_version, [], [AC_MSG_ERROR([libgit2 is required])])
AC_CHECK_LIB(curl, curl_easy_init, [], [AC_MSG_ERROR([libcurl is required])])
AC_CHECK_LIB(archive, archive_read_new, [], [AC_MSG_ERROR([libarchive is required])])

AC_CHECK_HEADER([json-c/json.h], AC_DEFINE(HAVE_JSON_C_H, 1, [Lib json-c has json-c header folder]))

AC_CHECK_HEADER([archive.h], [AC_DEFINE(HAVE_ARCHIVE_H, 1, [Lib archive has headers])], [AC_MSG_ERROR([archive.h is required])])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

AC_CHECK_HEADERS([libgen.h archive_entry.h curl/curl.h])

# Checks for library functions.
AC_CHECK_FUNCS([dladdr])

AC_OUTPUT

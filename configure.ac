#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([cpppm], [0.1], [c0der78@gmail.com])
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES(Makefile src/Makefile)
AM_INIT_AUTOMAKE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
AC_CHECK_LIB(json-c, json_tokener_parse)
AC_CHECK_LIB(git2, git_libgit2_version)
AC_CHECK_LIB(curl, curl_easy_init)
AC_CHECK_LIB(archive, archive_read_new)

if test "$ac_cv_lib_json_c_json_tokener_parse" = "no"; then
	AC_MSG_ERROR([libjson-c is required])
fi

if test "$ac_cv_lib_archive_archive_read_new" = "yes"; then
	AC_CHECK_HEADER([archive.h], [AC_DEFINE(HAVE_ARCHIVE_H, 1, [Lib archive has headers])], [AC_MSG_ERROR([archive.h is required])])
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.

AC_OUTPUT

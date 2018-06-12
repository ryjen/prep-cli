% PREP(1) Version 1.0 | prep documentation

NAME
====

**prep** — a modular package manager and build tool for c/c++ projects on posix systems.

SYNOPSIS
========

| **prep** \[_options_] **command** \[_arguments_]

DESCRIPTION
===========

The main benefits using prep are:

* Plugin architecture for extendability.  Plugins hook into different phases in the build cycle and support configuration and user interaction.

* Your project's dependencies can be a git repo, archive url or anything that a resolver plugin supports.

* It will work with any project's build system.  

* CMake integration for pure dependency management.


Options
-------

-h, --help

:   Prints brief usage information.

-l, --log _level_

:   Changes the log level.  Valid levels are: NONE, ERROR, WARN, INFO, DEBUG, and TRACE. 

--version

:   Prints the current version number.

-v, --verbose

:   Makes output from plugins and commands more verbose.

-c, --config _file_

:   Specifies the configuration file if its different than the default _package.json_.

-f, --force

:   Abstains from using the cache for commands.

-g, --global

:   Uses the global repository instead of the local one.

Commands
--------

get [_dependency_]

:   Gets all the project's dependencies or the optional specified _dependency_.  The dependencies must be defined in the configuration.

build [_dependency_]

:   Builds the current project or the specified _dependency_.  The _build system_ must be defined in the configuration.

test [_dependency_]

:   Tests the current project or the specified _dependency_.

install [_dependency_]

:   Installs the current project or the specified _dependency_ into the repository.

link <_dependency_>

:   Links a _dependency_ to the repository.

unlink <_dependency_>

:   Unlinks a _dependency_ from the repository.

cleanup

:   Removes build files and other intermediates from the repository.

env

:   Displays environment variables used by prep with the repository.

run

:   Run the executable defined for the current project.  Must be configured and installed in the repository.

plugins [_arguments_]

:   Starts the plugin manager.  Default action is to list plugins.  Use _help_ as an _argument_ to display other options for managing plugins.

FILES
=====

*package.json*

:   The default configuration file for a project.

*.prep*

:   A directory representing a repository for the current project.

*/usr/local/share/prep*

:   The global repository directory. 

ENVIRONMENT
===========

Prep supports passing environment variables to plugins.

The variables passed by default:

PATH, LD_LIBRARY_PATH, CXXFLAGS, LDFLAGS, PKG_CONFIG_PATH

BUGS
====

See GitHub Issues: <https://github.com/ryjen/prep/issues>

AUTHOR
======

Ryan Jennings <ryan@micrantha.com>

SEE ALSO
========

**prep-package(5)**, **prep-plugin(7)**
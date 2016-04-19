
prep
====

prep is a package manager for c/c++.  Being unsatisfied with the available tools for packages in this language.

Examples explain best.  This is prep's configuration:

```JSON
{
	"name": "prep",
	"version": 1.0,
	"author": {
		"name": "Ryan Jennings",
		"email": "c0der78@gmail.com"
	},
	"build_system": "autotools",
	"dependencies": [
		{
			"name": "libcurl",
			"location": "http://curl.haxx.se/download/curl-7.43.0.tar.bz2",
			"build_system": "autotools",
			"build_options": "--without-libidn --without-ssl --enable-darwinssl --disable-ldap",
			"dependencies": [
				{
					"name": "libz",
					"location": "http://zlib.net/zlib-1.2.8.tar.gz",
					"build_system": "autotools"
				}
			]
		},
		{
			"name": "json-c",
			"location": "https://github.com/json-c/json-c.git",
			"build_system": "autotools"
		},
		{
			"name": "libarchive",
			"location": "http://www.libarchive.org/downloads/libarchive-3.1.2.tar.gz",
			"build_system": "autotools",
			"dependencies": [
				{
					"name": "libxml2",
					"location": "http://xmlsoft.org/sources/libxml2-2.9.2.tar.gz",
					"build_system": "autotools",
					"build_options": "--without-python"
				},
				{
					"name": "iconv",
					"location": "",
					"build_system": "autotools"
				}
			]
		},
		{
			"name": "libgit2",
			"location": "https://github.com/libgit2/libgit2/archive/v0.23.1.tar.gz",
			"build_system": "cmake"
		}
	]

}
```

Features
========
- local user repository ($HOME/.prep)
- global repositories (/usr/local/prep)
- supports cmake, autoconf and makefile build systems
- dependency management

TODO
====
- ~plugin architecture for different build systems~ build_system = build_commands []
- custom build system
	- recursive json syntax
	- create visual studio projects on windows
	- create make files on nix, or perform compile tasks directly?
- package repository website/api (github?)
- move history/meta to sqlite database
- parse versions from filename
- store md5 hash of config in meta to detect changes

Repository Structure
====================

**.meta** : holds the version and package information

**.install** : holds a directory for each package installation files

**.history** : a record of incomplete installations

packages in **.install** are symlinked to **bin**, **lib**, **include**, etc.


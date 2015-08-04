
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
			"build_system": "autotools"
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

Package files can be used remotely for libraries that do not have one packaged.  For example:

```BASH

prep -f http://remote.host.com/package.json

```

or you can define a remote package file for a dependency like so:

```JSON
{
	"name": "somelib",
	"location": "http://some.download.url/file.archive",
	"package_file": "http://some.package.repo/package.json"
}
```

How It Works
============

Prep installs to a user repository ($HOME/.prep) by default.

You can specify a global repository (/usr/local/prep) with the '-g' option.

When prep builds a package, it will set environment variables to prefer the repository locations first.

If a build fails, a .history file is kept to resume appropriately.

When build succeeds, a record is kept in .meta/package_name which contains either the version or the download url.

This .meta/package is used to skip dependencies that are already installed.

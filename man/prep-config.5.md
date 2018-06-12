% PREP-CONFIG(5) Version 1.0 | prep documentation

NAME
====

| *package.json* : default configuration file for _prep(1)_

DESCRIPTION
===========

A JSON file that describes a prep project, its dependencies and how to build them.

SYNOPSIS
========

name

:     the name of the project or dependency.

version

:     the version of the project or dependency.

author

:     the author of the project.  may contain a name and/or an email

build_system

:     the plugins used to build the project specified sequentially

executable

:     specifies a generated binary for the project

dependencies

:     specifies a list of dependencies for a project

\<plugin\>

:     specifies custom plugin configuration

EXAMPLES
========

```
{
	"name": "prep",
	"version": "1.0",
	"author": {
		"name": "Ryan Jennings",
		"email": "ryan@micrantha.com"
	},
	"build_system": ["cmake", "make"],
	"executable": "prep",
	"dependencies": [
		{
			"name": "libarchive",
			"version": "3.3.2",
			"archive": {
				"location": "http://www.libarchive.org/downloads/libarchive-3.3.2.tar.gz"
			},
			"build_system": ["autotools", "make"]
		}
	]
}
```

In the above example, look at the libarchive dependency.  In order to resolve this, prep looks at the plugins.

For example, the **apt** plugin will try to install using the name **libarchive**.  

Should **apt** fail, the **archive** plugin is configured with a location to download and build from source.

BUGS
====

See GitHub Issues: <https://github.com/ryjen/prep/issues>

AUTHOR
======

Ryan Jennings <ryan@micrantha.com>

SEE ALSO
========

**prep(1)**, **prep-plugin(7)**



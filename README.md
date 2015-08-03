
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
			"build_system": "cmake"
		},
		{
			"name": "json-c",
			"location": "https://github.com/json-c/json-c.git",
			"build_system": "autotools"
		}
	}
	]

}

```

Package files can be used remotely for libraries that do not have one packaged.  For example:

```BASH

prep -f http://remote.host.com/package.json

```

Prep installs to a user resposiroty ($HOME/.prep) by default.  You can specify a global repository (/usr/local/prep) with the '-g' option.


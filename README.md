
prep
====

Prep is a modular package manager for c/c++.  Plugins are used for dependency management and build systems.  

Prep tries to follow NPM style repositories meaning there is a ***global repository*** (/usr/local/share/prep) and a ***current repository*** in the local directory.

I am no longer maintaining this project, so I'm releasing to open source.

plugins
=======

Plugins can be written in any language that supports stdin/stdout. Input parameters are read one line at a time on stdin.  All output is forwarded back to prep.   The plugins are forked to run in a seperate pseudo terminal. (See TODO for security)

The default plugins are currently written in Perl for prototyping purposes. Ideally this would be a compiled language.

There are two types of plugins **resolver** plugins and **build** plugins.

## plugin hooks:

#### load

occurs when a plugin is loaded for custom initialization

#### unload

occurs when a plugin is unloaded for custom cleanup

#### install

occurs when a dependency is installed.  Only affects plugins of type "resolver".

parameters: [**package, version**]

#### remove

occurs when a dependency is removed.  Only affects plugins of type "resolver".

parameters: [**package, version**]

#### build

occurs when a package is built. Only affects plugins of type "build".

parameters: [**package, version, sourcePath, buildPath, installPath, buildOpts, envVar=value... END**]

## current plugins:

- **archive**: extracts different archived formats
- **autotools**: configure script build system
- **cmake**: cmake build systems
- **git**: cloneing git resolver
- **homebrew**: install package resolver
- **make**: build system

## plugin manifest:

Plugins should contain a **manifest.json** to describe the type of plugin and how to run.

```JSON
{
    "executable": "main",
    "version": "0.1.0",
    "type": "resolver"
}
```

repository structure
====================

A repository by default is a **.prep** folder in the current directory.  By specifying the **-g** option, **/usr/local/share/prep** will be used instead.

Under the repository:

**/plugins** : holds all plugins

**/kitchen** : holds all file related to builds

**/kitchen/meta** : holds the version and package information

**/kitchen/install** : holds a directory for each package installation files

**/kitchen/build** : a separate directory for compiling

packages in **/kitchen/install** are symlinked to **bin**, **lib**, **include**, etc.


configuration
=====================

The configuration for a project is simple a **package.json** file containing the json.  The fields are as follows:

#### name
the name of the project

#### version
the version of the project 

#### build_system
an array of build plugins that define how to make a build.  So if your project uses cmake, you would define **cmake**, then **make**.

#### build_options
an array of options to pass directly to the build system.  You also have the option of using CPPFLAGS and LDFLAGS environment variables.

#### executable
the name of the executable to build

#### dependencies
an array of this configuration type objects defining each dependency.  Dependencies can be resolved using **resolver** plugins.  You can define multiple plugins on a dependency for example: **homebrew** to try and resolve a package, and **archive** to default resolve from an archive and build from source.

---
##### &lt;plugin&gt;
each resolver plugin can define its own options to override

---

### example:
This is what prep's configuration to build itself looks like:

```JSON
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
			"name": "libcurl",
			"homebrew": {
				"name": "curl"
			},
			"archive": {
				"location": "http://curl.haxx.se/download/curl-7.43.0.tar.bz2",
				"build_system": ["autotools", "make"],
				"build_options": "--without-libidn --without-ssl --enable-darwinssl --disable-ldap",
			},
			"dependencies": [
				{
					"name": "libz",
					"archive": {
						"location": "http://zlib.net/zlib-1.2.8.tar.gz",
						"build_system": ["cmake", "make"]
					}
				}
			]
		},
		{
			"name": "libarchive",
			"archive": {
				"location": "http://www.libarchive.org/downloads/libarchive-3.1.2.tar.gz",
				"build_system": ["autotools", "make"]
			},
			"dependencies": [
				{
					"name": "libxml2",
					"archive": {
						"location": "http://xmlsoft.org/sources/libxml2-2.9.2.tar.gz",
						"build_system": ["autotools", "make"],
						"build_options": "--without-python"
					}
				}
			]
		},
		{
			"name": "libgit2",
			"archive": {
				"location": "https://github.com/libgit2/libgit2/archive/v0.23.1.tar.gz",
				"build_system": ["cmake", "make"]
			}
		}
	]
}

```

!["Example Use Case"](screenshot.png)

TODO
====
- package repository website/api (ipfs?)
- parse archive versions from filename
- store md5 hash of config in meta to detect changes
- a way to rebuild a dependency or all dependencies
- a way to rebuild a package
- secure plugins (enforce digital signature?, chroot?)
- test suite
- convert plugins to compiled language



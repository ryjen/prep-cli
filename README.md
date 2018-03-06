
Prep
====

Prep is a modular package manager and build tool for c/c++ projects.  Yes, the core is written in C++ in the form of ```libutensil``` and ```prep``` binary, but the majority of the work is done by plugins.

**Prep will manage dependencies, paths and flags for building and running c/c++ projects.**

I don't have time to maintain this project, so I'm releasing to open source.

![Prep Building Itself](prep.gif)


Plugins
=======

Plugins can be written in **any language that supports stdin/stdout** using the following "crap point oh" version of a specification for communication. 

The plugins are forked to run in a seperate pseudo terminal. (See TODO for security)

-The default plugins are currently written in Perl for prototyping purposes, so right now there is a dependency on perl.  Ideally, they'd be compiled when prep is built.-


## plugin types:

#### dependency

resolves system packages

#### resolver

resolves package files for building

#### configuration

build system plugin for configuring a build

#### build

build system plugin for compiling

#### internal

Internal plugins are executed before any other plugin and cannot be specified in configuration.


## plugin hooks:

When writing a plugin, you have several **hooks** that will be sent over stdin.  Depending on what you need to do you can react to one or more of them.

#### LOAD

Occurs when a plugin is loaded for custom initialization

#### UNLOAD

Occurs when a plugin is unloaded for custom cleanup

#### ADD

Occurs when a dependency wants to be installed.  Only affects plugins of type "resolver".

parameters: [**package, version**]

#### REMOVE

Occurs when a dependency wants to be removed.  Only affects plugins of type "resolver".

parameters: [**package, version**]

#### BUILD

Occurs when a package wants to be built. Only affects plugins of type "build" and "configuration".

parameters: [**package, version, sourcePath, buildPath, installPath, buildOpts, envVar=value...**]

#### TEST

Occurs when a package wants to be tested. Only affects plugins of type "build".

parameters: [**package, version, sourcePath, buildPath, envVar=value...**]

#### INSTALL

Occurs when a package wants to be installed. Only affects plugins of type "build".

parameters: [**package, version, sourcePath, buildPath, envVar=value...**]


## plugin input header:

A plugin will always recieve a header on stdin:

```
<required hook>\n
<optional hook parameter>\n
<optional hook parameter>\n
END\n
```

## plugin output commands:

A plugin may send a command over stdout.


##### RETURN:

Specifies one or more return values.

```
RETURN <value>\n
```


##### ECHO:

Relays a message to prep, regardless of verbosity level.

```
ECHO <message>\n
```


Any other **output** by the plugin is forwarded to prep's output when in **verbose mode**.

## current plugins:

- **archive**: a resolver plugin that downloads and extracts different archived formats
- **autotools**: a build plugin that uses a configure script to generate makefiles. requires a configure script
- **cmake**: a build plugin that uses cmake to generate makefiles
- **git**: a resolver plugin that clones a git repository
- **homebrew**: a resolver plugin that installs packages using homebrew on OSX
- **make**: a build plugin that executes make on a makefile.  requires install param

## plugin manifest:

Plugins should contain a **manifest.json** to describe the type of plugin and how to run.

```JSON
{
    "executable": "main",
    "version": "0.1.0",
    "type": "resolver"
}
```

Repository Structure
====================

A repository by default is a **.prep** folder in the current directory.  By specifying the **-g** option, **/usr/local/share/prep** will be used instead (Inspired by node).

Under the repository:

**/plugins** : holds all plugins

**/kitchen** : holds all file related to builds

**/kitchen/meta** : holds the version and package information

**/kitchen/install** : holds a directory for each package installation files

**/kitchen/build** : a separate directory for compiling

packages in **/kitchen/install** are symlinked to **bin**, **lib**, **include** (etc) inside the repository and reused by prep.  You can add the repository to your path with ```prep setpath```  (TODO: Clarify this more)



Configuration
=============

The configuration for a project is simple a **package.json** file containing the json.  The fields are as follows:

#### name
the name of the project as a string

#### version
the version of the project as a string

#### build_system
an array of strings identifying build plugins that define how to make a build.  So if your project uses cmake, you would define **cmake**, then **make**.  the plugins are executed in the order of specified.

#### build_options
an array of strings to pass directly to the build system.  You have the option of using environment variables or compiler switches.

#### executable
the name of the executable or library to build as a string

#### dependencies
an array of this configuration type defining each dependency.  Dependencies will be resolved using **resolver** plugins in the order specified. Dependencies can also have dependencies. 

#### &lt;plugin&gt;
A plugin can define its own options to override.  For example if the **homebrew** plugin has a different name for the dependency you can specify it like:


```JSON
"dependencies": [
  {
	"name": "somelib",
	"homebrew": {
		"name": "altlibname"
	}
  }
],
```

### Example configuration:
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
			"name": "libarchive",
			"version": "3.3.2",
			"archive": {
				"location": "http://www.libarchive.org/downloads/libarchive-3.3.2.tar.gz"
			},
			"build_system": [
				"autotools",
				"make"
			]
		},
		{
			"name": "minisign",
			"version": "0.7",
			"archive": {
				"location": "https://github.com/jedisct1/minisign/archive/0.7.tar.gz"
			},
			"build_system": [
				"cmake", "make"
			],
			"dependencies": [
				{
					"name": "libsodium",
					"version": "1.0.15",
					"archive": {
						"location": "https://download.libsodium.org/libsodium/releases/libsodium-1.0.15.tar.gz"
					},
					"build_system": [
						"autotools", "make"
					]
				}
			]
		}
	]
}


```


TODO
====
- [ ] website/api for plugins and and configurations for common dependencies
- [-] parse archive versions from filename (2018-03-05 currently done in archive plugin)
- [ ] store md5 hash of configs in meta to detect changes
- [✓] store sub package.json in meta for dependencies
- [ ] a way to rebuild a dependency or all dependencies
- [ ] more security plugins (enforce digital signature?, chroot?)
- [ ] a way to install new plugins
- [ ] consider RPATH flags for runtime
- [ ] dependencies will be better as a tree rather than a list (need a good graph lib)
- [ ] consider sqlite storage
- [ ] test suite
- [✓] convert plugins to compiled language
- [ ] plugins may need to be interactive, currently not supported
- [-] repository cleanup (builds, old versions, etc) (2018-03-05 partially done)
- [ ] clion/intellij plugin
- [ ] ability for plugins to add commands to prep
- [ ] plugin sdks for languages
- [ ] package.json in subdirectory support (recursive)
- [ ] a strategy to lose dependency on 'prep run' (move library dependencies to system path)

Building
========

Just make sure you do a ```git submodule update --init --recursive```.  The rest I leave up to you. 

Contributing
============

Create an issue/feature, fork, build, send pull request.  Upon approval add your name to AUTHORS.
Also looking for maintainers, start a conversation.


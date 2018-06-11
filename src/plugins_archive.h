#ifndef MICRANTHA_PREP_PLUGINS_ARCHIVE_H
#define MICRANTHA_PREP_PLUGINS_ARCHIVE_H

#ifdef __APPLE__
#define DEFAULT_PLUGINS_LIB "libprep-default-plugins.dylib"
#else
#define DEFAULT_PLUGINS_LIB "libprep-default-plugins.so"
#endif

#define DEFAULT_PLUGINS_ARCHIVE "default_plugins_archive"
#define DEFAULT_PLUGINS_ARCHIVE_SIZE "default_plugins_archive_size"

extern "C" {
    typedef unsigned char *(*default_plugins_archive)();

    typedef size_t (*default_plugins_archive_size)();
}

#endif


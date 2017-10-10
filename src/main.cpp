
#include <unistd.h>
#include <iostream>
#include <vector>
#include <clocale>
#include <getopt.h>

#include "common.h"
#include "log.h"
#include "package_builder.h"
#include "util.h"
#include "vt100.h"

using namespace micrantha::prep;

void print_help(char *exe)
{
    printf("Syntax: %s [-g -f -v] install <package url, git url, plugin, archive or directory>\n", exe);
    printf("      : %s [-g -v] remove <package or plugin>\n", exe);
    printf("      : %s [-g -v] update <package or plugin>\n", exe);
    printf("      : %s [-g -v] link <package> [version]\n", exe);
    printf("      : %s [-g -v] unlink <package>\n", exe);
    printf("      : %s run\n", exe);
    printf("      : %s env\n", exe);
    printf("      : %s -h\n", exe);
    printf("      : %s check\n", exe);
}

int main(int argc, char *const argv[])
{
    PackageBuilder prep;
    Options options{.package_file = Repository::PACKAGE_FILE,
                    .global = false,
                    .location = ".",
                    .force_build = false,
                    .verbose = false,
                    .defaults = false};
    const char *command = nullptr;
    int option;
    int option_index = 0;
    static struct option args[] = {
            {"global",  no_argument,       0,  'g' },
            {"package", required_argument, 0,  'p' },
            {"force",   no_argument,       0,  'f' },
            {"verbose", no_argument,       0,  'v' },
            {"log",     required_argument, 0,  'l' },
            {"defaults",no_argument,       0,   1, },
            {"help",    no_argument,       0,   0  },
            {0,         0,                 0,   0  }
    };

    while ((option = getopt_long(argc, argv, "vhgfp:l:", args, &option_index)) != EOF) {
        switch (option) {
            case 'g':
                options.global = true;
                break;
            case 'p':
                options.package_file = optarg;
                break;
            case 'f':
                options.force_build = true;
                break;
            case 'v':
                options.verbose = true;
                break;
            case 'l':
                log::level::set(optarg);
                break;
            case 'h':
                print_help(argv[0]);
                return PREP_FAILURE;
            case 1:
                options.defaults = true;
                break;
            default:
                break;
        }
    }

    vt100::init();

    try {
        if (prep.initialize(options) != PREP_SUCCESS) {
            return PREP_FAILURE;
        }

    } catch (const std::exception &e) {
        log::error(e.what());
        return PREP_FAILURE;
    }

    vt100::disable_user();

    if (optind >= argc) {
        command = "install";
    } else {
        command = argv[optind++];
    }

    if (!strcmp(command, "env")) {
        prep.print_env();
        return PREP_SUCCESS;
    }

    try {
        if (prep.load(options) != PREP_SUCCESS) {
            return PREP_FAILURE;
        }

    } catch (const std::exception &e) {
        log::error(e.what());
        return PREP_FAILURE;
    }

    if (!strcmp(command, "install")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            char cwd[PATH_MAX] = {0};
            options.location = getcwd(cwd, sizeof(cwd)) ? cwd : ".";
        } else {
            options.location = argv[optind++];
        }

        auto callback = [&options](const Plugin::Result &result) { options.location = result.values.front(); };

        if (!directory_exists(options.location.c_str()) &&
            prep.repository()->notify_plugins_resolve(options.location, callback) == PREP_FAILURE) {
            log::error(options.location, " is not a valid prep package");
            return PREP_FAILURE;
        }

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config at ", options.location);
            return PREP_FAILURE;
        }

        return prep.build(config, options, options.location.c_str());
    }

    if (!strcmp(command, "remove")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Remove which package?");
            return PREP_FAILURE;
        }

        auto directory = prep.get_package_directory(argv[optind]);

        if (config.load(directory, options) == PREP_FAILURE) {
            log::error("unable to load config for ", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.remove(config, options);
    }


    if (!strcmp(command, "unlink")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Unlink which package?");
            return PREP_FAILURE;
        }
        if (config.load(argv[optind], options) == PREP_FAILURE) {
            log::error("unable to load config at ", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.unlink_package(config);
    }
    if (!strcmp(command, "link")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Link which package?");
            return PREP_FAILURE;
        }
        if (config.load(argv[optind], options) == PREP_FAILURE) {
            log::error("unable to load config at ", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.link_package(config);
    }

    if (!strcmp(command, "check")) {
        log::error("Not Implemented.");
        return PREP_FAILURE;
    }

    if (!strcmp(command, "run")) {
        PackageConfig config;
        char cwd[PATH_MAX];

        if (getcwd(cwd, sizeof(cwd))) {
            options.location = cwd;
        } else {
            options.location = ".";
        }

        auto callback = [&options](const Plugin::Result &result) { options.location = result.values.front(); };

        if (prep.repository()->notify_plugins_resolve(options.location, callback) == PREP_FAILURE) {
            micrantha::prep::log::error("%s is not a valid prep package", options.location.c_str());
            return PREP_FAILURE;
        }

        return prep.execute(config, argc - optind, &argv[optind]);
    }

    printf("Unknown command '%s'\n", command ? command : "null");

    print_help(argv[0]);
    return PREP_FAILURE;
}

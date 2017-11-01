
#include <getopt.h>
#include <unistd.h>
#include <clocale>
#include <iostream>
#include <vector>

#include "common.h"
#include "log.h"
#include "package_builder.h"
#include "util.h"
#include "vt100.h"

using namespace micrantha::prep;

void print_help(char *exe, const Options &options)
{
    printf("Syntax: %s install <package url, git url, plugin, archive or directory>\n", exe);
    printf("      : %s remove <package or plugin>\n", exe);
    printf("      : %s link <package> [version]\n", exe);
    printf("      : %s unlink <package>\n", exe);
    printf("      : %s run\n", exe);
    printf("      : %s env\n", exe);
    printf("      : %s check\n", exe);

    if (!options.verbose) {
        return;
    }

    printf("\n");
    printf("Options:\n");
    printf(" -h, --help     : displays this help");
    printf(" -f, --force    : forces an action");
    printf(" -g, --global   : uses the global repository for an action");
    printf(" -v, --verbose  : uses verbose output");
    printf("\n");
    printf("Commands:\n");
    printf("\n  install <package url, git url, plugin, archive or directory>\n");
    printf(
        "     resolves, builds, and installs a package or plugin to the repository.  "
        "argument can be a url to an archive, git repository\n");
    printf("\n  remove <package or plugin>\n");
    printf("     removes a package or plugin from the repository\n");
    printf("\n  link <package> [version]\n");
    printf("     links a built package to the path\n");
    printf("\n  unlink <package>\n");
    printf("     unlinks an installed package from the path\n");
    printf("\n  run\n");
    printf("     runs a built package in the path\n");
    printf("\n  env\n");
    printf("     prints out the repository environment\n");
    printf("\n  check\n");
    printf("     checks the repository structure for errors\n");
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
    static struct option args[] = {{"global", no_argument, 0, 'g'},
                                   {"package", required_argument, 0, 'p'},
                                   {"force", no_argument, 0, 'f'},
                                   {"verbose", no_argument, 0, 'v'},
                                   {"log", required_argument, 0, 'l'},
                                   {
                                       "defaults", no_argument, 0, 1,
                                   },
                                   {"help", no_argument, 0, 0},
                                   {0, 0, 0, 0}};

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
                print_help(argv[0], options);
                return PREP_FAILURE;
            case 1:
                options.defaults = true;
                break;
            default:
                break;
        }
    }

    vt100::init(options.verbose);

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

    print_help(argv[0], options);
    return PREP_FAILURE;
}

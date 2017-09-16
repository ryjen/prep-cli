
#include <iostream>
#include <unistd.h>
#include <vector>

#include "common.h"
#include "log.h"
#include "package_builder.h"
#include "util.h"

using namespace micrantha::prep;

void print_help(char *exe)
{
    printf("Syntax: %s [-g -f] install <package url, git url, archive or directory>\n", exe);
    printf("      : %s [-g] remove <package>\n", exe);
    printf("      : %s [-g] update <package>\n", exe);
    printf("      : %s [-g] link <package> [version]\n", exe);
    printf("      : %s [-g] unlink <package>\n", exe);
    printf("      : %s run\n", exe);
    printf("      : %s -h\n", exe);
    printf("      : %s check\n", exe);
}

int main(int argc, char *const argv[])
{
    micrantha::prep::package_builder prep;
    micrantha::prep::options options{
            .package_file = repository::PACKAGE_FILE,
            .global = false,
            .location = ".",
            .force_build = false
    };
    const char *command;
    int option;

    while ((option = getopt(argc, argv, "hgfp:l:")) != EOF) {
        switch (option) {
        case 'g':
            options.global = true;
            break;
        case 'p':
            options.package_file = optarg;
            break;
        case 'f':
            options.force_build = true;
        case 'l':
            micrantha::prep::set_log_level(optarg);
            break;
        case 'h':
            print_help(argv[0]);
            return PREP_FAILURE;
            default:break;
        }
    }

    try {
        if (prep.initialize(options)) {
            return PREP_FAILURE;
        }

    } catch (const std::exception &e) {
        micrantha::prep::log_error(e.what());
        return PREP_FAILURE;
    }

    if (optind >= argc) {
        command = "install";
    } else {
        command = argv[optind++];
    }

    if (!strcmp(command, "install")) {
        micrantha::prep::package_config config;

        if (optind < 0 || optind >= argc) {
            char cwd[PATH_MAX] = {0};
            options.location = getcwd(cwd, sizeof(cwd)) ? cwd : ".";
        } else {
            options.location = argv[optind++];
        }

        auto callback = [&options](const plugin::Result &result) {
            options.location = result.values.front();
        };

        if (!directory_exists(options.location.c_str()) &&
            prep.repository()->notify_plugins_resolve(options.location, callback) == PREP_FAILURE) {
            micrantha::prep::log_error("%s is not a valid prep package", options.location.c_str());
            return PREP_FAILURE;
        }

        if (config.load(options.location, options) == PREP_FAILURE) {
            micrantha::prep::log_error("unable to load config at %s", options.location.c_str());
            return PREP_FAILURE;
        }

        return prep.build(config, options, options.location.c_str());
    }

    if (!strcmp(command, "remove")) {
        micrantha::prep::package_config config;

        if (optind < 0 || optind >= argc) {
            micrantha::prep::log_error("Remove which package?");
            return PREP_FAILURE;
        }

        return prep.remove(config, options, argv[optind]);
    }

    if (!strcmp(command, "setpath")) {

        prep.add_path_to_shell();

        return PREP_SUCCESS;
    }

    if (!strcmp(command, "unlink")) {

        micrantha::prep::package_config config;

        if (optind < 0 || optind >= argc) {
            micrantha::prep::log_error("Unlink which package?");
            return PREP_FAILURE;
        }
        if (config.load(argv[optind], options) == PREP_FAILURE) {
            micrantha::prep::log_error("unable to load config at %s", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.unlink_package(config);
    }
    if (!strcmp(command, "link")) {

        micrantha::prep::package_config config;

        if (optind < 0 || optind >= argc) {
            micrantha::prep::log_error("Link which package?");
            return PREP_FAILURE;
        }
        if (config.load(argv[optind], options) == PREP_FAILURE) {
            micrantha::prep::log_error("unable to load config at %s", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.link_package(config);
    }

    if (!strcmp(command, "check")) {
    }

    if (!strcmp(command, "run")) {
        micrantha::prep::package_config config;
        char cwd[PATH_MAX];

        if (getcwd(cwd, sizeof(cwd))) {
            options.location = cwd;
        } else {
            options.location = ".";
        }
        auto callback = [&options](const plugin::Result &result) {
            options.location = result.values.front();
        };

        if (prep.repository()->notify_plugins_resolve(options.location, callback) == PREP_FAILURE) {
            micrantha::prep::log_error("%s is not a valid prep package", options.location.c_str());
            return PREP_FAILURE;
        }

        return prep.execute(config, argc - optind, &argv[optind]);
    }

    printf("Unknown command '%s'\n", command ? command : "null");

    print_help(argv[0]);
    return PREP_FAILURE;
}

#include <unistd.h>
#include <iostream>
#include "common.h"
#include "package_builder.h"
#include "log.h"
#include "package_resolver.h"
#include "util.h"

using namespace arg3::prep;

void print_help(char *exe)
{
    printf("Syntax: %s [-g -f] install <package url, git url, archive or directory>\n", exe);
    printf("      : %s [-g] remove <package>\n", exe);
    printf("      : %s [-g] update <package>\n", exe);
    printf("      : %s -h\n", exe);
    printf("      : %s check\n", exe);
}

int main(int argc, char *const argv[])
{
    arg3::prep::package_builder prep;
    arg3::prep::options options;
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
                arg3::prep::set_log_level(optarg);
                break;
            case 'h':
                print_help(argv[0]);
                return PREP_FAILURE;
        }
    }

    try {
        if (prep.initialize(options)) {
            return PREP_FAILURE;
        }

    } catch (const std::exception &e) {
        arg3::prep::log_error(e.what());
        return PREP_FAILURE;
    }

    if (optind >= argc) {
        command = "install";
    } else {
        command = argv[optind++];
    }

    if (!strcmp(command, "install")) {
        arg3::prep::package_resolver resolver;
        arg3::prep::package_config config;
        char cwd[PATH_MAX];

        if (optind < 0 || optind >= argc) {
            if (getcwd(cwd, sizeof(cwd))) {
                options.location = cwd;
            } else {
                options.location = ".";
            }

        } else {
            options.location = argv[optind++];
        }

        if (resolver.resolve_package(config, options)) {
            arg3::prep::log_error("%s is not a valid prep package", options.location.c_str());
            return PREP_FAILURE;
        }

        return prep.build(config, options, resolver.package_dir().c_str());
    }

    if (!strcmp(command, "remove")) {
        arg3::prep::package_resolver resolver;
        arg3::prep::package_config config;

        if (optind < 0 || optind >= argc) {
            arg3::prep::log_error("Remove which package?");
            return PREP_FAILURE;
        }

        return prep.remove(config, options, argv[optind]);
    }

    if (!strcmp(command, "setpath")) {
        arg3::prep::repository repo;

        if (repo.initialize(options)) {
            arg3::prep::log_error("unable to initialize repository");
            return PREP_FAILURE;
        }

        prompt_to_add_path_to_shell_rc(".zshrc", repo.get_bin_path().c_str());
        if (prompt_to_add_path_to_shell_rc(".bash_profile", repo.get_bin_path().c_str())) {
            prompt_to_add_path_to_shell_rc(".bashrc", repo.get_bin_path().c_str());
        }
        prompt_to_add_path_to_shell_rc(".kshrc", repo.get_bin_path().c_str());

        return PREP_SUCCESS;
    }

    if (!strcmp(command, "check")) {
    }

    printf("Unknown command '%s'\n", command ? command : "null");

    print_help(argv[0]);
    return PREP_FAILURE;
}

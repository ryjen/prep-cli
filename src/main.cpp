#include <iostream>
#include <unistd.h>
#include "package_builder.h"
#include "package_resolver.h"
#include "log.h"
#include "common.h"

void print_help(char *exe)
{
    printf("Syntax: %s [-g] install <package url, git url, archive or directory>\n", exe);
    printf("      : %s [-g] remove <package>\n", exe);
    printf("      : %s [-g] update <package>\n", exe);
    printf("      : %s check\n", exe);
}

int main(int argc, char *const argv[])
{
    arg3::prep::package_builder prep;
    arg3::prep::options options;
    const char *command;
    int option;

    while ((option = getopt(argc, argv, "gfp:l:")) != EOF)
    {
        switch (option)
        {
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
        }
    }

    try
    {
        if (prep.initialize(options) ) {
            return PREP_FAILURE;
        }

    }
    catch (const std::exception &e)
    {
        arg3::prep::log_error(e.what());
        return PREP_FAILURE;
    }

    if (optind >= argc)
    {
        command = "install";
    }
    else
    {
        command = argv[optind++];
    }

    if (!strcmp(command, "install"))
    {
        arg3::prep::package_resolver resolver;
        arg3::prep::package_config config;

        if (optind < 0 || optind >= argc)
        {
            options.location = ".";
        }
        else
        {
            options.location = argv[optind++];
        }

        if (resolver.resolve_package(config, options))
        {
            arg3::prep::log_error("%s is not a valid prep package", options.location.c_str());
            return PREP_FAILURE;
        }

        return prep.build(config, options, resolver.working_dir().c_str());
    }

    if (!strcmp(command, "remove"))
    {
        arg3::prep::package_resolver resolver;
        arg3::prep::package_config config;

        if (optind < 0 || optind >= argc)
        {
            arg3::prep::log_error("Remove which package?");
            return PREP_FAILURE;
        }

        if (resolver.resolve_existing_package(config, options, argv[optind]))
        {
            if (prep.remove(argv[optind], options)) {
                arg3::prep::log_error("%s is not an existing package", argv[optind]);
                return PREP_FAILURE;
            }

            return PREP_SUCCESS;
        }

        return prep.remove(config, options);
    }

    if (!strcmp(command, "check"))
    {
    }

    printf("Unknown command '%s'\n", command ? command : "null");

    print_help(argv[0]);
    return PREP_FAILURE;
}
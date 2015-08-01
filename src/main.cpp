#include <iostream>
#include <unistd.h>
#include "package_builder.h"
#include "package_resolver.h"
#include "log.h"

void print_help(char *exe)
{
    printf("Syntax: %s [-g] install <package>\n", exe);
    printf("      : %s [-g] remove <package>\n", exe);
    printf("      : %s [-g] update <package>\n", exe);
    printf("      : %s check\n", exe);
}

int main(int argc, char *const argv[], char *const envp[])
{
    arg3::prep::package_builder prep;
    arg3::prep::options options;
    const char *command;
    int option;

    while ((option = getopt(argc, argv, "gf:")) != EOF)
    {
        switch (option)
        {
        case 'g':
            options.global = true;
            break;
        case 'f':
            options.package_file = optarg;
            break;
        }
    }

    try
    {
        prep.initialize(options);
    }
    catch (const std::exception &e)
    {
        arg3::prep::log_error(e.what());
        return EXIT_FAILURE;
    }

    if (optind == 1)
    {
        command = "install";
    }
    else  if (optind >= argc)
    {
        print_help(argv[0]);
        return EXIT_FAILURE;
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

        if (resolver.resolve_package(config, options, options.location))
        {
            arg3::prep::log_error("%s is not a valid prep package", options.location.c_str());
            return EXIT_FAILURE;
        }

        return prep.build(config, options, resolver.working_dir().c_str());
    }

    if (!strcmp(command, "check"))
    {
    }


    return EXIT_SUCCESS;
}
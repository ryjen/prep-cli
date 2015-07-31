#include <iostream>
#include "package_builder.h"
#include "package_resolver.h"
#include <unistd.h>

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

    const char *command;

    int option;

    while ((option = getopt(argc, argv, "g")) != EOF)
    {
        switch (option)
        {
        case 'g':
            prep.set_global(true);
            break;
        }
    }

    try
    {
        prep.initialize();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    if (optind == 1)
    {
        command = "install";
    }
    else if (optind >= argc)
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
            resolver.set_location(".");
        }
        else
        {
            resolver.set_location(argv[optind++]);
        }

        if (resolver.resolve_package(&config))
        {
            printf("Error %s is not a valid prep package\n", resolver.location().c_str());
            return EXIT_FAILURE;
        }

        return prep.build(config, resolver.working_dir().c_str());
    }
    else if (!strcmp(command, "check"))
    {
    }


    return EXIT_SUCCESS;
}
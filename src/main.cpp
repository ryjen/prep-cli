#include <iostream>
#include "repository.h"
#include "package_resolver.h"
#include <unistd.h>

void print_help(char *exe)
{
    printf("Syntax: %s [-g] install <package>\n", exe);
    printf("      : %s [-g] remove <package>\n", exe);
    printf("      : %s [-g] update <package>\n", exe);
    printf("      : %s check\n", exe);
}

int main(int argc, char *const argv[])
{
    arg3::prep::repository repo;

    int option;

    while ((option = getopt(argc, argv, "g")) != EOF)
    {
        switch (option)
        {
        case 'g':
            repo.set_global(true);
            break;
        }
    }

    try
    {
        repo.initialize();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    const char *command;

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

        if (optind < 0 || optind >= argc)
        {
            resolver.set_arg(".");
        }
        else
        {
            resolver.set_arg(argv[optind++]);
        }

        arg3::prep::package_config config;

        if (resolver.resolve_package(config))
        {
            printf("Error %s is not a valid prep package\n", resolver.arg().c_str());
            return EXIT_FAILURE;
        }

        return repo.get(config);

    }
    else if (!strcmp(command, "check"))
    {
    }


    return EXIT_SUCCESS;
}
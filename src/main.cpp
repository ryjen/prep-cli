#include <iostream>
#include "repository.h"
#include <unistd.h>

void print_help()
{
    printf("Syntax: cpppm [-g] install <package>\n");
    printf("      : cpppm [-g] remove <package>\n");
    printf("      : cpppm [-g] update <package>\n");
    printf("      : cpppm check\n");
}
int main(int argc, char *const argv[])
{
    arg3::cpppm::repository  repo;

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
        return 1;
    }

    if (optind < 2 && optind >= argc)
    {
        print_help();
        return 1;
    }

    char *command = argv[optind++];

    if (!strcmp(command, "install"))
    {
        arg3::cpppm::package_config config;

        if (optind < 0 || optind >= argc)
        {
            config.set_path(".");
        }
        else
        {
            config.set_path(argv[optind++]);
        }

        if (config.load())
        {
            return repo.get(config);
        }
        else
        {
            printf("Error %s is not a valid cpppm package\n", config.path().c_str());
            return 1;
        }
    }
    else if (!strcmp(command, "check"))
    {
        return 0;
    }


    return 0;
}
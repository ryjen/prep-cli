#include <iostream>
#include "repository.h"
#include <unistd.h>

void print_help()
{
    printf("Syntax: cpppm [-g] install <package>\n");
    printf("      : cpppm [-g] remove <package>\n");
    printf("      : cpppm [-g] update <package>\n");
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

    if (optind < 0 && optind >= argc)
    {
        print_help();
        return 1;
    }

    char *command = argv[optind++];

    if (!strcmp(command, "install"))
    {
        if (optind < 0 || optind >= argc)
        {
            printf("Please specify a git url");
            return 1;
        }
        repo.get(argv[optind++]);
    }


    return 0;
}
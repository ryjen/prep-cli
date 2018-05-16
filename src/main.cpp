
#include <getopt.h>
#include <clocale>
#include <iostream>
#include <vector>

#include "common.h"
#include "log.h"
#include "controller.h"
#include "options.h"
#include "util.h"

using namespace micrantha::prep;

void print_help(char *exe, const Options &options) {

    //TODO replace with man page
    printf("Syntax: %s build [package]\n", exe);
    printf("        %s test [package]\n", exe);
    printf("        %s install [package]\n", exe);
    printf("        %s get [package]\n", exe);
    printf("        %s add <package or plugin>\n", exe);
    printf("        %s remove <package or plugin>\n", exe);
    printf("        %s link <package> [version]\n", exe);
    printf("        %s unlink <package>\n", exe);
    printf("        %s cleanup [package]\n", exe);
    printf("        %s run\n", exe);
    printf("        %s env\n", exe);
    printf("        %s check\n", exe);

    if (options.verbose != Verbosity::All) {
        return;
    }

    printf("\n");
    printf("Options:\n");
    printf(" -h, --help     : displays this help");
    printf(" -f, --force    : forces an action");
    printf(" -g, --global   : uses the global repository for an action");
    printf(" -v, --verbose  : uses verbose output");
    printf("\n");
    printf("Concepts:\n");
    printf(" <package> can be a package url, git url, plugin, archive file or directory\n\n");
    printf("Commands:\n");
    printf("\n  build [package]\n");
    printf("     resolves and builds a package or plugin to the repository.\n");
    printf("\n  test [package]\n");
    printf("     test a package or plugin that has already been built.\n");
    printf("\n  install [package]\n");
    printf("     installs a package or plugin to the repository.\n");
    printf("\n  get [package]\n");
    printf("     gets a package or packages for dependencies only.\n");
    printf("\n  add <package or plugin>\n");
    printf("     adds a package or plugin to the repository\n");
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
    printf("\n  cleanup [package]\n");
    printf("     cleans temp files and folders for a package or the entire repository\n");
}

int main(int argc, char *const argv[]) {
    Controller prep;
    Options options{
            .package_file = Repository::PACKAGE_FILE,
            .global = false,
            .location = ".",
            .force_build = ForceLevel::None,
            .verbose = Verbosity::None,
            .defaults = false};
    const char *command = nullptr;
    int option;
    int option_index = 0;
    static struct option opts[] = {{"global",   no_argument,       nullptr, 'g'},
                                   {"package",  required_argument, nullptr, 'p'},
                                   {"force",    no_argument,       nullptr, 'f'},
                                   {"verbose",  optional_argument, nullptr, 'v'},
                                   {"log",      required_argument, nullptr, 'l'},
                                   {"defaults", no_argument,       nullptr, 1},
                                   {"help",     no_argument,       nullptr, 'h'},
                                   {nullptr,    0,           nullptr, 0}};

    while ((option = getopt_long(argc, argv, "vhgfp:l:", opts, &option_index)) != EOF) {
        switch (option) {
            case 'g':
                options.global = true;
                break;
            case 'p':
                options.package_file = optarg;
                break;
            case 'f':
                options.force_build = ForceLevel::All;
                break;
            case 'v':
                options.verbose = Verbosity::All;
                break;
            case 'l':
                if (!log::level::set(optarg)) {
                    puts("Unknown log level");
                    return PREP_FAILURE;
                }
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

    vt100::init();

    try {
        if (prep.initialize(options) != PREP_SUCCESS) {
            return PREP_FAILURE;
        }
    } catch (const std::exception &e) {
        log::error(e.what());
        return PREP_FAILURE;
    }

    if (optind == argc) {
        command = "get";
    } else {
        command = argv[optind++];

        if (options.force_build == ForceLevel::None) {
            options.force_build = ForceLevel::Project;
        }
        if (options.verbose == Verbosity::None) {
            options.verbose = Verbosity::Project;
        }
    }

    if (!strcmp(command, "env")) {
        if (optind < argc) {
            return prep.print_env(argv[optind]);
        } else {
            return prep.print_env(nullptr);
        }
    }

    if (!strcmp(command, "unlink")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Unlink which package?");
            return PREP_FAILURE;
        }
        if (config.load(argv[optind], options) == PREP_FAILURE) {
            log::error("unable to load config for ", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.unlink(config);
    }
    if (!strcmp(command, "link")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Link which package?");
            return PREP_FAILURE;
        }
        if (config.load(argv[optind], options) == PREP_FAILURE) {
            log::error("unable to load config for ", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.link(config);
    }

    if (!strcmp(command, "check")) {
        log::error("Not Implemented.");
        return PREP_FAILURE;
    }

    if (!strcmp(command, "run")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        return prep.execute(config, argc - optind, &argv[optind]);
    }

    try {
        if (prep.load(options) != PREP_SUCCESS) {
            return PREP_FAILURE;
        }

    } catch (const std::exception &e) {
        log::error(e.what());
        return PREP_FAILURE;
    }

    if (!strcmp(command, "cleanup") || !strcmp(command, "clean")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        return prep.cleanup(config, options);
    }

    if (!strcmp(command, "build")) {
        PackageConfig config;

        // load a package.json config
        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        // current package only...
        if (optind == argc) {
            // build
            return prep.build(config, options, options.location);
        }

        auto dep = config.find_dependency(argv[optind++]);

        if (!dep) {
            log::error("no such dependency");
            return PREP_FAILURE;
        }

        return prep.build(*dep, options, options.location);
    }

    if (!strcmp(command, "get")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        if (optind == argc) {
            return prep.get(config, options, options.location);
        }

        auto dep = config.find_dependency(argv[optind++]);

        if (!dep) {
            log::error("no such dependency");
            return PREP_FAILURE;
        }
        return prep.get(*dep, options, options.location);
    }

    if (!strcmp(command, "test")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", argv[optind]);
            return PREP_FAILURE;
        }

        if (optind == argc) {
            // build
            return prep.test(config, options);
        }

        auto dep = config.find_dependency(argv[optind++]);

        if (!dep) {
            log::error("no such dependency");
            return PREP_FAILURE;
        }

        return prep.test(*dep, options);
    }

    if (!strcmp(command, "install")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", argv[optind]);
            return PREP_FAILURE;
        }

        return prep.install(config, options);
    }

    if (!strcmp(command, "add")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            char cwd[PATH_MAX] = {0};
            options.location = getcwd(cwd, sizeof(cwd)) ? cwd : ".";
        } else {
            // get from a url/directory
            options.location = argv[optind++];
        }

        // if not a directory...
        if (directory_exists(options.location) != PREP_SUCCESS) {

            // try to resolve to a directory
            auto result = prep.repository()->notify_plugins_resolve(options.location);

            if (result == PREP_FAILURE || result.values.empty()) {
                log::error(options.location, " is not a valid prep package");
                return PREP_FAILURE;
            }

            options.location = result.values.front();
        }

        // load a package.json config
        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        // build
        if (prep.build(config, options, options.location) == PREP_FAILURE) {
            return PREP_FAILURE;
        }

        if (prep.test(config, options) == PREP_FAILURE) {
            return PREP_FAILURE;
        }

        // install
        return prep.install(config, options);
    }

    if (!strcmp(command, "remove")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Remove which package?");
            return PREP_FAILURE;
        }

        auto directory = prep.get_package_directory(argv[optind++]);

        if (config.load(directory, options) == PREP_FAILURE) {
            log::error("unable to load config for ", directory);
            return PREP_FAILURE;
        }

        return prep.remove(config, options);
    }

    printf("Unknown command '%s'\n", command ? command : "null");

    print_help(argv[0], options);
    return PREP_FAILURE;
}

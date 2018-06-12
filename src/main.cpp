
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

void print_help(const Options &options) {


    if (options.verbose != Verbosity::All) {
        io::println(color::g("Syntax"), ": ", options.exe, " build [package]");
        io::println(std::setw(12), options.exe, " test [package]");
        io::println(std::setw(12), options.exe, " install [package]");
        io::println(std::setw(12), options.exe, " get [package]");
        io::println(std::setw(12), options.exe, " add [package]");
        io::println(std::setw(12), options.exe, " remove [package]");
        io::println(std::setw(12), options.exe, " link <package> [version]");
        io::println(std::setw(12), options.exe, " unlink <package>");
        io::println(std::setw(12), options.exe, " cleanup [package]");
        io::println(std::setw(12), options.exe, " plugins [options...]");
        io::println(std::setw(12), options.exe, " run");
        io::println(std::setw(12), options.exe, " env");
        io::println(std::setw(12), options.exe, " check");
        return;
    }

    execlp("man", "prep", "prep", nullptr);
}

int main(int argc, char *const argv[]) {
    Controller prep;
    Options options{
            .exe = argv[0],
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
                                   {"config",  required_argument, nullptr, 'c'},
                                   {"force",    no_argument,       nullptr, 'f'},
                                   {"verbose",  optional_argument, nullptr, 'v'},
                                   {"log",      required_argument, nullptr, 'l'},
                                   {"defaults", no_argument,       nullptr, 1},
                                   {"help",     no_argument,       nullptr, 'h'},
                                   {nullptr,    0,           nullptr, 0}};

    while ((option = getopt_long(argc, argv, "vhgfc:l:", opts, &option_index)) != EOF) {
        switch (option) {
            case 'g':
                options.global = true;
                break;
            case 'c':
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
                print_help(options);
                return PREP_FAILURE;
            case 1:
                options.defaults = true;
                break;
            default:
                break;
        }
    }

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

    if (string::equals(command, "env")) {
        if (optind < argc) {
            return prep.print_env(argv[optind]);
        } else {
            return prep.print_env(nullptr);
        }
    }

    if (string::equals(command, "unlink")) {
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
    if (string::equals(command, "link")) {
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

    if (string::equals(command, "check")) {
        log::error("Not Implemented.");
        return PREP_FAILURE;
    }

    if (string::equals(command, "run")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        return prep.execute(config, argc - optind, &argv[optind]);
    }

    if (string::equals(command, "plugins")) {

        return prep.plugins(options, argc - optind, &argv[optind]);
    }

    try {
        if (prep.load(options) != PREP_SUCCESS) {
            return PREP_FAILURE;
        }

    } catch (const std::exception &e) {
        log::error(e.what());
        return PREP_FAILURE;
    }

    if (string::equals(command, "cleanup") || string::equals(command, "clean")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", options.location);
            return PREP_FAILURE;
        }

        return prep.cleanup(config, options);
    }

    if (string::equals(command, "build")) {
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

    if (string::equals(command, "get")) {
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

    if (string::equals(command, "test")) {
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

    if (string::equals(command, "install")) {
        PackageConfig config;

        if (config.load(options.location, options) == PREP_FAILURE) {
            log::error("unable to load config for ", argv[optind]);
            return PREP_FAILURE;
        }

        if (optind == argc) {
            return prep.install(config, options);
        }

        auto dep = config.find_dependency(argv[optind++]);

        if (!dep) {
            log::error("no such dependency");
            return PREP_FAILURE;
        }

        return prep.install(*dep, options);
    }

    if (string::equals(command, "add")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            char cwd[PATH_MAX] = {0};
            options.location = getcwd(cwd, sizeof(cwd)) ? cwd : ".";
        } else {
            // get from a url/directory
            options.location = argv[optind++];
        }

        // if not a directory...
        if (filesystem::directory_exists(options.location) != PREP_SUCCESS) {

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

    if (string::equals(command, "remove")) {
        PackageConfig config;

        if (optind < 0 || optind >= argc) {
            log::error("Remove which package or plugin?");
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

    print_help(options);
    return PREP_FAILURE;
}

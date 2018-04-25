//
// Created by Ryan Jennings on 2018-03-05.
//

#ifndef PREP_OPTIONS_H
#define PREP_OPTIONS_H

namespace micrantha {

    namespace prep {

        enum class ForceLevel {
            None,
            Project,
            All
        };

        enum class Verbosity {
            None,
            Project,
            All
        };

        /**
         * options passed from the command line
         */
        typedef struct {
            // use global repository
            bool global;
            // the package file/name
            std::string package_file;
            // the package location
            std::string location;
            // forces a build
            ForceLevel force_build;
            // display plugin output
            Verbosity verbose;
            // accept default options
            bool defaults;
        } Options;

    }
}

#endif //PREP_OPTIONS_H

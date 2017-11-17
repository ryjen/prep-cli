#ifndef MICRANTHA_PREP_COMMON_H
#define MICRANTHA_PREP_COMMON_H

/**
 * indicates a successful return value
 */
#define PREP_SUCCESS 0

/**
 * indicates a failure return value
 */
#define PREP_FAILURE 1

/**
 * indicates an error return value
 */
#define PREP_ERROR -1


namespace micrantha
{
    namespace prep
    {
        //! the local user repository location
        extern const char *const LOCAL_REPO_NAME;

        //! the global repository location
        extern const char *const GLOBAL_REPO;
    }
}

#endif


find_program(PREP_PROGRAM prep)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PREP
        FOUND_VAR PREP_FOUND
        REQUIRED_VARS PREP_PROGRAM)

function(prep_prefix)

    if (NOT PREP_FOUND)
        return()
    endif()

    set(options)
    set(singleValueOpts DIRECTORY)
    set(multiValueOpts)

    cmake_parse_arguments(PREP_PREFIX "${options}" "${singleValueOpts}" "${multiValueOpts}" ${ARGN})

    if (NOT PREP_PREFIX_DIRECTORY)
        message(FATAL_ERROR "DIRECTORY is required for prep prefix")
    endif()

    exec_program(${PREP_PROGRAM} ${PREP_PREFIX_DIRECTORY} ARGS env prefix OUTPUT_VARIABLE PREP_INSTALL_PREFIX)

    set(CMAKE_PREFIX_PATH "${PREP_INSTALL_PREFIX}:${CMAKE_PREFIX_PATH}")

    exec_program(${PREP_PROGRAM} ${PREP_PREFIX_DIRECTORY} ARGS env cxxflags OUTPUT_VARIABLE PREP_CXX_FLAGS)

    exec_program(${PREP_PROGRAM} ${PREP_PREFIX_DIRECTORY} ARGS env ld_library_path OUTPUT_VARIABLE PREP_LINKER_PATH)

    add_compile_options(${PREP_CXX_FLAGS})

    link_directories(${PREP_LINKER_PATH})

    message(STATUS "Using prep repository at ${PREP_INSTALL_PREFIX}")
endfunction()

mark_as_advanced(PREP_PROGRAM PREP_INSTALL_PREFIX)
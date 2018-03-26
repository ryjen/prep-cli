
find_program(PREP_PROGRAM prep)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(PREP
        FOUND_VAR PREP_FOUND
        REQUIRED_VARS PREP_PROGRAM)

function(prep_init)

    if (NOT PREP_FOUND)
        return()
    endif()

    set(options)
    set(singleValueOpts)
    set(multiValueOpts)

    cmake_parse_arguments(PREP_INIT "${options}" "${singleValueOpts}" "${multiValueOpts}" ${ARGN})

    set(PREP_INIT_PACKAGE_FILE ${CMAKE_CURRENT_LIST_DIR}/package.json)

    if (NOT EXISTS ${PREP_INIT_PACKAGE_FILE})
        message(FATAL_ERROR "${PREP_GET_DIRECTORY} should contain a package.json")
    endif()

    set(PREP_INIT_REPOSITORY ${CMAKE_CURRENT_LIST_DIR})

    exec_program(${PREP_PROGRAM} ${PREP_INIT_REPOSITORY} ARGS env prefix OUTPUT_VARIABLE PREP_INSTALL_PREFIX)

    set(CMAKE_PREFIX_PATH "${PREP_INSTALL_PREFIX}:${CMAKE_PREFIX_PATH}")

    exec_program(${PREP_PROGRAM} ${PREP_INIT_REPOSITORY} ARGS env cxxflags OUTPUT_VARIABLE PREP_CXX_FLAGS)

    exec_program(${PREP_PROGRAM} ${PREP_INIT_REPOSITORY} ARGS env ld_library_path OUTPUT_VARIABLE PREP_LINKER_PATH)

    add_compile_options(${PREP_CXX_FLAGS})

    link_directories(${PREP_LINKER_PATH})

    message(STATUS "Using prep repository at ${PREP_INSTALL_PREFIX}")

    add_custom_target(prep-dependencies COMMAND ${PREP_PROGRAM} -f get
            WORKING_DIRECTORY "${PREP_INIT_REPOSITORY}"
            DEPENDS ${PREP_INIT_PACKAGE_FILE})

    set(PREP_INIT_DEPENDENCIES prep-dependencies PARENT_SCOPE)
endfunction()

mark_as_advanced(PREP_PROGRAM PREP_INSTALL_PREFIX PREP_INIT_DEPENDENCIES)
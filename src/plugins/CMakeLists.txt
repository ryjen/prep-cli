
project(${PLUGINS_FOLDER} Go)

function(add_plugin)
    set(options)
    set(singleValueOpts FOLDER ROOT MANIFEST EXECUTABLE)

    cmake_parse_arguments(ADD_PLUGIN "${options}" "${singleValueOpts}" "${multiValueOpts}" ${ARGN})


    if (NOT DEFINED ADD_PLUGIN_FOLDER)
        message(FATAL_ERROR "FOLDER name for add_plugin not specified")
    endif()

    if (NOT DEFINED ADD_PLUGIN_ROOT)
        if (NOT DEFINED PROJECT_NAME)
            message(FATAL_ERROR "ROOT folder for add_plugin not specified")
        endif()
        set(ADD_PLUGIN_ROOT ${PROJECT_NAME})
    endif()


    if (NOT DEFINED ADD_PLUGIN_MANIFEST)
        set(ADD_PLUGIN_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/manifest.json")
    endif()

    if (NOT DEFINED ADD_PLUGIN_EXECUTABLE)
        set(ADD_PLUGIN_EXECUTABLE "${CMAKE_CURRENT_BINARY_DIR}/main")
    endif()

    file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/gen/${ADD_PLUGIN_ROOT}/${ADD_PLUGIN_FOLDER})

    add_custom_command(TARGET main POST_BUILD COMMAND ${CMAKE_COMMAND} ARGS -E copy ${ADD_PLUGIN_MANIFEST} ${ADD_PLUGIN_EXECUTABLE} ${CMAKE_BINARY_DIR}/gen/${ADD_PLUGIN_ROOT}/${ADD_PLUGIN_FOLDER})

    add_dependencies(ZIP_PLUGINS main)

endfunction()


add_subdirectory(support)

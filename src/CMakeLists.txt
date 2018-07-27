
# define source files for the binary
set(SOURCE_FILES
    controller.cpp
    package.cpp
    plugin.cpp
    repository.cpp
    environment.cpp
    log.cpp
    util.cpp
    decompressor.cpp
    vt100.cpp
    plugin_manager.cpp
)

#defines the library header files for installation
set(HEADER_FILES
    common.h
    environment.h
    plugins_archive.h
    log.h
    util.h
    decompressor.h
    vt100.h
    plugin_manager.h
)

#---------------------------------------------------------------------------------------------------------
# prep binary

#define the executable
add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES} ${HEADER_FILES})

#link libraries to the executable
target_link_libraries(${PROJECT_NAME} ${LibArchive_LDFLAGS} ${CMAKE_DL_LIBS} ${LIB_UTIL})

#add include directories
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${LibArchive_INCLUDE_DIRS})

#---------------------------------------------------------------------------------------------------------
# default plugins shared library

# Generate source for default plugins
set(PLUGINS_ARCHIVE_SOURCE ${CMAKE_CURRENT_LIST_DIR}/plugins_archive.cpp)

# generate the plugins archive source file
add_custom_command(OUTPUT ${PLUGINS_ARCHIVE_SOURCE} COMMAND ${CMAKE_COMMAND} -DWORKING_DIR=${PROJECT_BINARY_DIR} -P
    ${CMAKE_CURRENT_LIST_DIR}/CMakeGenerateSource.txt
        DEPENDS bin2csv build_plugins )

add_custom_target(generate_source ALL DEPENDS ${PLUGINS_ARCHIVE_SOURCE})

set_source_files_properties(${PLUGINS_ARCHIVE_SOURCE} PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/plugins_archive.cpp.in)

# the shared library name
set(DEFAULT_PLUGINS ${PROJECT_NAME}-default-plugins)

# define the default plugins shared library
add_library(${DEFAULT_PLUGINS} SHARED ${PLUGINS_ARCHIVE_SOURCE})

add_dependencies(${DEFAULT_PLUGINS} generate_source)

#---------------------------------------------------------------------------------------------------------
# installation

#sets installation for library and executable binaries
install(TARGETS ${PROJECT_NAME} ${DEFAULT_PLUGINS}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
)



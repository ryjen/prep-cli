
# define source files for the library
set(LIBRARY_FILES
    environment.cpp
    log.cpp
    util.cpp
    decompressor.cpp
    vt100.cpp)

# define source files for the binary
set(SOURCE_FILES
    package_builder.cpp
    package_config.cpp
    plugin.cpp
    repository.cpp)

include_directories(${LibArchive_INCLUDE_DIRS})

#define the library
add_library(${PROJECT_LIBRARY} ${LIBRARY_FILES})

#define the executable
add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})

#link libraries to the executable
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARY} ${LibArchive_LDFLAGS})

#add a target to zip the plugins.  requires the zip command on the path
add_custom_target(ZIP_PLUGINS COMMAND ${ZIP_EXECUTABLE} -r ${PROJECT_BINARY_DIR}/gen/plugins.zip .
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/plugins)

#add a target to convert the zip file to a csv of bytes we can include in the source
#sets the zip file target as a dependency
add_custom_target(GEN_PLUGIN_INC COMMAND bin2csv ${PROJECT_BINARY_DIR}/gen/plugins.zip
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins.inc
        DEPENDS ZIP_PLUGINS)

#add the plugins bytes as a dependency on the library target
add_dependencies(${PROJECT_LIBRARY} GEN_PLUGIN_INC)

#defines the library header files for installation
set(HEADER_FILES
    common.h
    environment.h
    log.h
    util.h
    decompressor.h
    vt100.h)

#sets installation target for library header files
install(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_LIBRARY})

#sets installation for library and executable binaries
install(TARGETS ${PROJECT_LIBRARY} ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

#set installation of plugins into shared folder
install(DIRECTORY plugins DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)

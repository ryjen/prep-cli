
# define source files for the library
set(LIBRARY_FILES
    environment.cpp
    log.cpp
    util.cpp
    decompressor.cpp
    vt100.cpp)

# define source files for the binary
set(SOURCE_FILES
    controller.cpp
    package.cpp
    plugin.cpp
    plugins_archive.cpp
    repository.cpp)

include_directories(${LibArchive_INCLUDE_DIRS})

#define the library
add_library(${PROJECT_LIBRARY} ${LIBRARY_FILES})

#define the executable
add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})

#link libraries to the executable
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARY} ${LibArchive_LDFLAGS})

set(PLUGINS_ARCHIVE_SOURCE ${CMAKE_CURRENT_LIST_DIR}/plugins_archive.cpp)

# generate the plugins archive source file
add_custom_command(OUTPUT ${PLUGINS_ARCHIVE_SOURCE} COMMAND ${CMAKE_COMMAND} -DWORKING_DIR=${PROJECT_BINARY_DIR} -P
    ${CMAKE_CURRENT_LIST_DIR}/CMakeGenerateSource.txt
        DEPENDS bin2csv build_plugins )

add_custom_target(generate_source ALL DEPENDS ${PLUGINS_ARCHIVE_SOURCE})

set_source_files_properties(${PLUGINS_ARCHIVE_SOURCE} PROPERTIES OBJECT_DEPENDS ${CMAKE_CURRENT_LIST_DIR}/plugins_archive.cpp.in)

add_dependencies(${PROJECT_NAME} generate_source)

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



#---------------------------------------------------------------------------------------------------------
# define source files
#---------------------------------------------------------------------------------------------------------

# for the binary
set(SOURCE_FILES
    controller.cpp
    package.cpp
    plugin.cpp
    repository.cpp
    plugin_manager.cpp
)

# the library
set(LIBRARY_FILES
    log.cpp
    environment.cpp
    util.cpp
    decompressor.cpp
    vt100.cpp
)

#---------------------------------------------------------------------------------------------------------
#  define the header files
#---------------------------------------------------------------------------------------------------------

set(HEADER_FILES
    common.h
    plugins_archive.h
    plugin_manager.h
)

set(LIBRARY_HEADERS
    environment.h
    log.h
    util.h
    decompressor.h
    vt100.h
)

#---------------------------------------------------------------------------------------------------------
# define outputs
#---------------------------------------------------------------------------------------------------------

#define the library
add_library(${PROJECT_LIBRARY} ${LIBRARY_FILES} ${LIBRARY_HEADERS})

target_include_directories(${PROJECT_LIBRARY} SYSTEM PUBLIC ${LibArchive_INCLUDE_DIRS})

#---------------------------------------------------------------------------------------------------------
# prep binary

# define the executable
add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES} ${HEADER_FILES})

# link libraries to the executable
target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARY} ${LibArchive_LDFLAGS} ${CMAKE_DL_LIBS} ${LIB_UTIL})

# add include directories
target_include_directories(${PROJECT_NAME} SYSTEM PUBLIC ${LibArchive_INCLUDE_DIRS})

add_dependencies(${PROJECT_NAME} ${PROJECT_LIBRARY})

#---------------------------------------------------------------------------------------------------------
# default plugins shared library
#---------------------------------------------------------------------------------------------------------

if (NOT PACKAGER_PROGRAM)

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
#---------------------------------------------------------------------------------------------------------

  #sets installation for library and executable binaries
  install(TARGETS ${PROJECT_NAME} ${DEFAULT_PLUGINS}
      RUNTIME DESTINATION bin
      LIBRARY DESTINATION lib
  )

else()

  # just install the binary
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

endif()


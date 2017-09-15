
set(LIBRARY_FILES
    environment.cpp
    log.cpp
    util.cpp
    decompressor.cpp)

set(SOURCE_FILES
    package_builder.cpp
    package_config.cpp
    plugin.cpp
    repository.cpp)

include_directories(${LibArchive_INCLUDE_DIRS})

add_library(${PROJECT_LIBRARY} ${LIBRARY_FILES})

add_executable(${PROJECT_NAME} main.cpp ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARY} ${ZLIB_LIBRARIES} ${LibArchive_LIBRARIES} ${CURL_LIBRARIES}
        ${GIT2_LIBRARIES} ${ICONV_LIBRARIES})

add_custom_target(ZIP_PLUGINS COMMAND zip -r ${PROJECT_BINARY_DIR}/gen/plugins.zip .
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/plugins)

add_custom_target(GEN_PLUGIN_INC COMMAND bin2csv ${PROJECT_BINARY_DIR}/gen/plugins.zip
        ${CMAKE_CURRENT_SOURCE_DIR}/plugins.inc
        DEPENDS ZIP_PLUGINS)

add_dependencies(${PROJECT_LIBRARY} GEN_PLUGIN_INC)

set(HEADER_FILES
    common.h
    environment.h
    log.h
    package_builder.h
    package_config.h
    plugin.h
    repository.h
    util.h
)

INSTALL(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})

INSTALL(TARGETS ${PROJECT_LIBRARY} ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

INSTALL(DIRECTORY plugins DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME} USE_SOURCE_PERMISSIONS)

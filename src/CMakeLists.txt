
set(SOURCE_FILES
    decompressor.cpp
    environment.cpp
    log.cpp
    package_builder.cpp
    package_config.cpp
    package_dependency.cpp
    package_resolver.cpp
    plugin.cpp
    repository.cpp
    util.cpp
)

include_directories(${LibArchive_INCLUDE_DIRS})

add_library(${PROJECT_LIBRARY} ${SOURCE_FILES})

add_executable(${PROJECT_NAME} main.cpp)

target_link_libraries(${PROJECT_NAME} ${PROJECT_LIBRARY} ${ZLIB_LIBRARIES} ${JSON_C_LIBRARIES} ${LibArchive_LIBRARIES} ${CURL_LIBRARIES} ${GIT2_LIBRARIES} ${ICONV_LIBRARIES})

set(HEADER_FILES
    common.h
    decompressor.h
    environment.h
    log.h
    package_builder.h
    package_config.h
    package_resolver.h
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

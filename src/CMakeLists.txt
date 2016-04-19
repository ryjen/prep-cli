
set(SOURCE_FILES 
    decompressor.cpp
    environment.cpp
    log.cpp
    package_builder.cpp
    package_config.cpp
    package_dependency.cpp
    package_resolver.cpp
    repository.cpp
    util.cpp
)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

add_executable(${PROJECT_NAME}per main.cpp)

target_link_libraries(${PROJECT_NAME}per ${PROJECT_NAME})

set(HEADER_FILES
    common.h 
    decompressor.h 
    environment.h 
    log.h 
    package_builder.h 
    packacge_config.h 
    package_resolver.h 
    repository.h 
    util.h
)

INSTALL(FILES ${HEADER_FILES} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/${PROJECT_NAME})


include(ExternalProject)

externalproject_add(bandit
        GIT_REPOSITORY https://github.com/banditcpp/bandit.git
        GIT_TAG v2.0.0
        INSTALL_COMMAND ""
        BUILD_COMMAND ""
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/bandit)

add_executable(${PROJECT_NAME}-test main.test.cpp util.test.cpp ../src/util.cpp)

ExternalProject_Get_property(bandit SOURCE_DIR)

target_include_directories(${PROJECT_NAME}-test SYSTEM PUBLIC ${SOURCE_DIR} PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries (${PROJECT_NAME}-test ${PROJECT_LIBRARY})


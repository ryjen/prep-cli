
#---------------------------------------------------------------------------------------------------------
# Setup build dependencies
#---------------------------------------------------------------------------------------------------------
include(ExternalProject)

externalproject_add(bandit
        GIT_REPOSITORY https://github.com/banditcpp/bandit.git
        GIT_TAG v2.0.0
        INSTALL_COMMAND ""
        BUILD_COMMAND ""
        PREFIX ${CMAKE_CURRENT_BINARY_DIR}/bandit)

ExternalProject_Get_property(bandit SOURCE_DIR)

set(BANDIT_SOURCE_DIR ${SOURCE_DIR})

#---------------------------------------------------------------------------------------------------------
# setup test executable
#---------------------------------------------------------------------------------------------------------

add_executable(${PROJECT_NAME}-test main.test.cpp util.test.cpp ../src/util.cpp)

target_include_directories(${PROJECT_NAME}-test SYSTEM PUBLIC ${BANDIT_SOURCE_DIR} PUBLIC ${PROJECT_SOURCE_DIR}/src)

target_link_libraries (${PROJECT_NAME}-test ${PROJECT_LIBRARY} ${LIB_UTIL} ${LIB_FTS})

add_dependencies(${PROJECT_NAME}-test bandit)

#---------------------------------------------------------------------------------------------------------
# Add the main executable test
#---------------------------------------------------------------------------------------------------------

if (MEMORY_CHECK)
	include(ValgrindTest)
	add_valgrind_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
else ()
	add_test(${PROJECT_NAME}-test ${PROJECT_BINARY_DIR}/tests/${PROJECT_NAME}-test)
endif()


# add target for code coverage
if(CODE_COVERAGE)
  include(CodeCoverage)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_CXX_FLAGS_COVERAGE}")
  setup_target_for_coverage(${PROJECT_NAME}-coverage ${PROJECT_SOURCE_DIR}/coverage)
endif()


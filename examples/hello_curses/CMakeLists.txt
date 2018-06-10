
cmake_minimum_required (VERSION 3.2)

# define project name
project (hello_curses VERSION 0.1.0)

find_package(Prep)

set(CURSES_NEED_NCURSES TRUE)

find_package(Curses REQUIRED)

add_executable(hello hello.c)

target_include_directories(hello SYSTEM PUBLIC ${CURSES_INCLUDE_DIR})

target_link_libraries(hello ${CURSES_LIBRARIES})

if (PREP_FOUND)
    prep_init()
    add_dependencies(hello ${PREP_INIT_DEPENDENCIES})
endif()

install(TARGETS hello RUNTIME DESTINATION bin)


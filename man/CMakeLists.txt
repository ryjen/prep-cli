
find_program(PANDOC_BINARY pandoc)

if (NOT PANDOC_BINARY)
    message(WARNING "pandoc command not found, man pages will not be generated!")
    return()
endif()

set(PANDOC_FLAGS --standalone --to man --from markdown)

set(MAN_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/share/man)

set(PREP_MAN ${CMAKE_CURRENT_BINARY_DIR}/prep.1)

add_custom_command(OUTPUT ${PREP_MAN} COMMAND ${PANDOC_BINARY} ${PANDOC_FLAGS} -o ${PREP_MAN}
        ${CMAKE_CURRENT_LIST_DIR}/prep.1.md DEPENDS ${CMAKE_CURRENT_LIST_DIR}/prep.1.md)

install(FILES ${PREP_MAN} DESTINATION ${MAN_INSTALL_PREFIX}/man1)

set(PREP_CONFIG_MAN ${CMAKE_CURRENT_BINARY_DIR}/prep-config.5)

add_custom_command(OUTPUT ${PREP_CONFIG_MAN} COMMAND ${PANDOC_BINARY} ${PANDOC_FLAGS} -o ${PREP_CONFIG_MAN}
        ${CMAKE_CURRENT_LIST_DIR}/prep-config.5.md DEPENDS ${CMAKE_CURRENT_LIST_DIR}/prep-config.5.md)

install(FILES ${PREP_CONFIG_MAN} DESTINATION ${MAN_INSTALL_PREFIX}/man5)

set(PREP_PLUGIN_MAN ${CMAKE_CURRENT_BINARY_DIR}/prep-plugin.7)

add_custom_command(OUTPUT ${PREP_PLUGIN_MAN} COMMAND ${PANDOC_BINARY} ${PANDOC_FLAGS} -o ${PREP_PLUGIN_MAN}
        ${CMAKE_CURRENT_LIST_DIR}/prep-plugin.7.md DEPENDS ${CMAKE_CURRENT_LIST_DIR}/prep-plugin.7.md)

install(FILES ${PREP_PLUGIN_MAN} DESTINATION ${MAN_INSTALL_PREFIX}/man7)

add_custom_target(man ALL DEPENDS ${PREP_MAN} ${PREP_CONFIG_MAN} ${PREP_PLUGIN_MAN})


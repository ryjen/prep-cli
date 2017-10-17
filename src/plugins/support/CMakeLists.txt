
set(SOURCE_FILES plugin.go)

add_executable(main ${SOURCE_FILES})

add_plugin(FOLDER "support")
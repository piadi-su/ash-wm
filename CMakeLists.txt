cmake_minimum_required(VERSION 3.16)

project(ashwm C)

add_executable(ashwm 
	src/main.c
)


target_compile_options(ashwm PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(ashwm PRIVATE
    X11
	Xinerama
	Xcursor
)

install(TARGETS ashwm
    RUNTIME DESTINATION bin
)


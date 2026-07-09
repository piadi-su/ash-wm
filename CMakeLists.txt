cmake_minimum_required(VERSION 3.16)

project(ash-wm C)

add_executable(ash-wm 
	src/main.c
)

# target_include_directories(ash-wm PRIVATE
#     /usr/include/freetype2
#     /usr/include/fontconfig
#     /usr/include
# )

target_compile_options(ash-wm PRIVATE
    -Wall
    -Wextra
    -Wpedantic
)

target_link_libraries(ash-wm PRIVATE
    X11
	Xinerama
    # Xft
    # fontconfig
    # freetype
    # m
    # Xrender
	# xcb
	#    xcb-util
	#    xcb-keysyms
	#    xcb-randr
	#    xcb-ewmh
)

# ===== INSTALL RULE =====
# install(TARGETS ash-wm
#     RUNTIME DESTINATION bin
# )


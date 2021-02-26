cmake_minimum_required(VERSION 3.14)
project(dbsdr C)

set(CMAKE_C_STANDARD 11)

include_directories(/usr/include)

find_package(PkgConfig REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenGL REQUIRED)

pkg_search_module(GLFW REQUIRED glfw3)


add_executable(dbsdr main.c global.h mouse.c mouse.h device.c device.h shader.c
        shader.h queue.c queue.h fft.c fft.h waterfall.c waterfall.h linmath.h window.c window.h game_state.c game_state.h)
target_link_libraries(dbsdr GLEW::GLEW ${GLFW_STATIC_LIBRARIES}
        ${OPENGL_LIBRARIES} hackrf pthread fftw3 stb m)

add_custom_command(
        TARGET dbsdr POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        ${CMAKE_BINARY_DIR}/assets)

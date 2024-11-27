cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)

project(ToodeeSculpt)

find_package(glfw3 3.3 REQUIRED)

add_executable(ToodeeSculpt
./src/App.cpp
./src/Editor.cpp
./src/Icons.cpp
./src/main.mm
./src/MetalLayerHelper.mm
./src/MouseCursors.cpp
./src/PrimitivesStack.cpp
./src/renderer/Renderer.cpp
./src/renderer/shader_reader.c
./src/system/format.c
./src/system/log.c
./src/system/microui.c
./src/system/point_in.c
./src/system/sokol_time.c
./src/system/undo.c
)

target_link_libraries(ToodeeSculpt glfw)
target_link_libraries(ToodeeSculpt
        "-framework Metal"
        "-framework MetalKit"
        "-framework AppKit"
        "-framework Foundation"
        "-framework QuartzCore"
        )

target_compile_options(ToodeeSculpt PRIVATE -O3 -Wall -Wextra -Wno-missing-braces)
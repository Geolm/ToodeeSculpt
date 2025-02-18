cmake_minimum_required(VERSION 3.25)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 11)
set(CMAKE_DEBUG_FLAGS "-g -O0")
set(CMAKE_RELEASE_FLAGS "-O3 -DNDEBUG -DSHADERS_IN_EXECUTABLE -Wall -Wextra -Wno-missing-braces")

project(ToodeeSculpt)

# --- Build the binarize ---
add_executable(binarize ./src/tools/binarize.c ./src/system/bin2h.c ./src/system/format.c ./src/renderer/shader_reader.c)

# --- Prebuild step ---
add_custom_command(
        OUTPUT bin_output.txt
        WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        COMMAND binarize > bin_output.txt
        DEPENDS binarize
)

# --- Build ToodeeSculpt ---

find_package(glfw3 3.3 REQUIRED)

add_executable(ToodeeSculpt
./src/App.cpp
./src/Icons.c
./src/main.mm
./src/MetalLayerHelper.mm
./src/MouseCursors.cpp
./src/editor/color_box.c
./src/editor/Editor.cpp
./src/editor/export.c
./src/editor/primitive.c
./src/editor/primitive_list.c
./src/editor/PrimitiveEditor.cpp
./src/renderer/Renderer.cpp
./src/renderer/shader_reader.c
./src/system/arc.c
./src/system/biarc.c
./src/system/color.c
./src/system/format.c
./src/system/log.c
./src/system/microui.c
./src/system/ortho.c
./src/system/palettes.c
./src/system/point_in.c
./src/system/sokol_time.c
./src/system/undo.c
./src/system/whereami.c
)

add_custom_target(prebuild_step DEPENDS bin_output.txt)
add_dependencies(ToodeeSculpt prebuild_step)

target_link_libraries(ToodeeSculpt glfw)
target_link_libraries(ToodeeSculpt
        "-framework Metal"
        "-framework MetalKit"
        "-framework AppKit"
        "-framework Foundation"
        "-framework QuartzCore"
        ${CMAKE_SOURCE_DIR}/lib/libnfd.a
        )

target_compile_options(ToodeeSculpt PRIVATE
        $<$<CONFIG:Debug>:-g -O0>
        $<$<CONFIG:Release>:-O3 -DNDEBUG -DSHADERS_IN_EXECUTABLE -Wall -Wextra -Wno-missing-braces>
)
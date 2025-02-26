cmake_minimum_required(VERSION 3.28)

# CLion configuration. Note that Ninja does NOT work with c++ 20 for some unknown reason...
# -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE=/usr/local/emsdk/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake -DCMAKE_BUILD_TYPE=Debug

set(wgpu-shader-toy_RELEASE_YEAR  "2025")
set(wgpu-shader-toy_RELEASE_MONTH "02"  )
set(wgpu-shader-toy_RELEASE_DAY   "26"  )

set(wgpu-shader-toy_VERSION "${wgpu-shader-toy_RELEASE_YEAR}.${wgpu-shader-toy_RELEASE_MONTH}.${wgpu-shader-toy_RELEASE_DAY}")

project(wgpu_shader_toy LANGUAGES CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "This project must be compiled with emscripten")
endif()

set(CMAKE_WARN_UNUSED OFF)

set(CMAKE_CXX_STANDARD 23)

set(wgpu_shader_toy_sources
    src/cpp/Application.h
    src/cpp/Application.cpp
    src/cpp/FragmentShader.h
    src/cpp/FragmentShader.cpp
    src/cpp/FragmentShaderWindow.h
    src/cpp/FragmentShaderExamples.cpp
    src/cpp/FragmentShaderWindow.cpp
    src/cpp/Errors.h
    src/cpp/MainWindow.h
    src/cpp/MainWindow.cpp
    src/cpp/MainWindowActions.cpp
    src/cpp/Preferences.h
    src/cpp/Preferences.cpp
    src/cpp/State.h
    src/cpp/fmt.h

    src/cpp/main.cpp

    src/cpp/gui/Dialog.h
    src/cpp/gui/Dialog.cpp
    src/cpp/gui/WstGui.h
    src/cpp/gui/WstGui.cpp

    src/cpp/gpu/GPU.h
    src/cpp/gpu/GPU.cpp
    src/cpp/gpu/ImGuiWindow.h
    src/cpp/gpu/ImGuiWindow.cpp
    src/cpp/gpu/Renderable.h
    src/cpp/gpu/Window.h
    src/cpp/gpu/Window.cpp

    src/cpp/utils/Clock.h
    src/cpp/utils/DataManager.h
    src/cpp/utils/DataManager.cpp
    src/cpp/utils/JSStorage.cpp
    src/cpp/utils/Storage.h
    src/cpp/utils/UndoManager.h
    src/cpp/utils/UndoManager.cpp

    external/santaclose/ImGuiColorTextEdit/TextEditor.cpp
)

set(VERSION_DIR "${CMAKE_BINARY_DIR}/generated")
configure_file("${CMAKE_CURRENT_LIST_DIR}/src/cpp/version.h.in" "${VERSION_DIR}/version.h")

add_executable(wgpu_shader_toy "${wgpu_shader_toy_sources}")

set_target_properties(wgpu_shader_toy PROPERTIES OUTPUT_NAME "index")
target_include_directories(wgpu_shader_toy PUBLIC
    "${VERSION_DIR}"
    "external/nlohmann/json/single_include"
    "external/santaclose/ImGuiColorTextEdit"
    "external/fonts/src"
)

file(COPY ${CMAKE_SOURCE_DIR}/src/resources/images DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/src/resources/shaders DESTINATION ${CMAKE_BINARY_DIR})

# Copy (and processes) shell.html
set(SHELL_SRC "${CMAKE_SOURCE_DIR}/src/resources/emscripten/shell.html")
set(SHELL_DST "${CMAKE_BINARY_DIR}/index.html")

add_custom_command(
    OUTPUT ${SHELL_DST}
    COMMAND ${CMAKE_COMMAND} -D SRC=${SHELL_SRC} -D DST=${SHELL_DST} -D wgpu-shader-toy_VERSION=${wgpu-shader-toy_VERSION} -P ${CMAKE_CURRENT_LIST_DIR}/CopyResource.cmake
    DEPENDS ${SHELL_SRC}
)
add_custom_target(copy_shell_html DEPENDS ${SHELL_DST})

set(IMGUI_DISABLE_DEMO "$<IF:$<CONFIG:Debug>,false,true>")
set(PORTS_OPTIMIZATION_LEVEL "$<IF:$<CONFIG:Debug>,0,2>")

#set(wgpu_shader_toy_options "--use-port=contrib.glfw3" "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/imgui.py:backend=glfw:renderer=wgpu:disableDemo=$<IF:$<CONFIG:Debug>,false,true>")
set(wgpu_shader_toy_options
    "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/emscripten-glfw3.py:optimizationLevel=${PORTS_OPTIMIZATION_LEVEL}:disableWebGL2=true"
    "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/imgui.py:backend=glfw:renderer=wgpu:disableDemo=${IMGUI_DISABLE_DEMO}:optimizationLevel=${PORTS_OPTIMIZATION_LEVEL}:disableDefaultFont=true")

target_compile_options(wgpu_shader_toy PUBLIC
    "${wgpu_shader_toy_options}"
    "-fwasm-exceptions"
)

target_link_options(wgpu_shader_toy PUBLIC
    "-fwasm-exceptions"
    "${wgpu_shader_toy_options}"
    "--js-library" "${CMAKE_CURRENT_LIST_DIR}/src/js/lib_wgpu_shader_toy.js"
)

add_dependencies(wgpu_shader_toy copy_shell_html)
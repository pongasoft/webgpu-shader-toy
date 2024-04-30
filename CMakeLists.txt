cmake_minimum_required(VERSION 3.28)

project(wgpu_shader_toy LANGUAGES CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "This project must be compiled with emscripten")
endif()

set(CMAKE_WARN_UNUSED OFF)

set(CMAKE_CXX_STANDARD 20)

set(wgpu_shader_toy_sources
    src/cpp/Application.h
    src/cpp/Application.cpp
    src/cpp/FragmentShaderWindow.h
    src/cpp/FragmentShaderWindow.cpp
    src/cpp/Errors.h
    src/cpp/MainWindow.h
    src/cpp/MainWindow.cpp
    src/cpp/Model.h
    src/cpp/Preferences.h
    src/cpp/Preferences.cpp
    src/cpp/fmt.h

    src/cpp/main.cpp

    src/cpp/gpu/GPU.h
    src/cpp/gpu/GPU.cpp
    src/cpp/gpu/ImGuiWindow.h
    src/cpp/gpu/ImGuiWindow.cpp
    src/cpp/gpu/Renderable.h
    src/cpp/gpu/Window.h
    src/cpp/gpu/Window.cpp

    src/cpp/utils/Storage.h
    src/cpp/utils/JSStorage.cpp
    src/cpp/utils/Utils.h
)

add_executable(wgpu_shader_toy "${wgpu_shader_toy_sources}")

set_target_properties(wgpu_shader_toy PROPERTIES OUTPUT_NAME "index")
set_target_properties(wgpu_shader_toy PROPERTIES SUFFIX ".html")
target_include_directories(wgpu_shader_toy PUBLIC
    "external/nlohmann/json/single_include"
)

set(wgpu_shader_toy_options "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/imgui.py:backend=glfw:renderer=wgpu:branch=docking")

target_compile_options(wgpu_shader_toy PUBLIC
    "${wgpu_shader_toy_options}"
    "-fwasm-exceptions"
)

target_link_options(wgpu_shader_toy PUBLIC
    "-fwasm-exceptions"
    "--shell-file=${CMAKE_CURRENT_LIST_DIR}/src/resources/emscripten/shell.html"
    "${wgpu_shader_toy_options}"
)

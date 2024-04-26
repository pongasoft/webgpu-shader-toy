cmake_minimum_required(VERSION 3.28)

project(wgpu_shader_toy LANGUAGES CXX)

if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "This project must be compiled with emscripten")
endif()

set(CMAKE_CXX_STANDARD 20)

set(wgpu_shader_toy_sources
    src/cpp/Application.h
    src/cpp/Application.cpp
    src/cpp/Errors.h
    src/cpp/fmt.h
    src/cpp/GPU.cpp
    src/cpp/GPU.h
    src/cpp/ShaderWindow.h
    src/cpp/ShaderWindow.cpp
    src/cpp/main.cpp
)

add_executable(wgpu_shader_toy "${wgpu_shader_toy_sources}")

set_target_properties(wgpu_shader_toy PROPERTIES OUTPUT_NAME "index")
set_target_properties(wgpu_shader_toy PROPERTIES SUFFIX ".html")

set(wgpu_shader_toy_options "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/imgui.py:backend=glfw:renderer=wgpu:branch=docking")

target_compile_options(wgpu_shader_toy PUBLIC
    "${wgpu_shader_toy_options}"
)

target_link_options(wgpu_shader_toy PUBLIC
    "--shell-file=${CMAKE_CURRENT_LIST_DIR}/src/resources/emscripten/shell.html"
    "-sNO_DISABLE_EXCEPTION_CATCHING"
    "${wgpu_shader_toy_options}"
)

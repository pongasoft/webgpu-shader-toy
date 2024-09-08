Introduction
------------

WebGPU Shader Toy is a free and open source tool for experimenting with WebGPU fragment shaders and the WebGPU Shader Language (WGSL)

#wasm #webgpu

Tool
----

The tool is free and live [WebGPU Shader Toy](https://pongasoft.github.io/webgpu-shader-toy/)!

<a href="https://pongasoft.github.io/webgpu-shader-toy/"><img width="1269" alt="webgpu-shader-toy-v2024 07 29" src="https://github.com/user-attachments/assets/1365457f-d82d-467f-8519-116ce8529e63"></a>

No ads. No Tracking. No server-side component.

There is a [video](https://www.youtube.com/watch?v=DZjFbFUaxy8) on YouTube showing some of the primary features.

Project
-------

> [!NOTE]
> This section describes the **project** itself, not the **product**.
> Check the WebGPU Shader Toy [documentation](https://pongasoft.com/webgpu-shader-toy/index.html) for information about the tool and its behavior.

### History
This project originated from my interest in learning about WebGPU. It also served as a platform for developing a more comprehensive application for my GLFW emscripten implementation: [emscripten-glfw](https://github.com/pongasoft/emscripten-glfw).

### Technologies used

* [emscripten](https://emscripten.org/) (web assembly) for the compiler
* [ImGui](https://github.com/ocornut/imgui)
  * ImGui Backend: GLFW with [emscripten-glfw](https://github.com/pongasoft/emscripten-glfw) implementation (`--use-port=contrib.glfw3`)
  * ImGui Renderer: WebGPU (`-sUSE_WEBGPU=1`)

This tool uses [emscripten-glfw](https://github.com/pongasoft/emscripten-glfw) with its multi (GLFW) window support:

* The pane containing the controls and code editor is a window using ImGui (with a GLFW backend).
  The code driving this window can be found in the file [MainWindow.cpp](src/cpp/MainWindow.cpp)
* The pane rendering the shader is a GLFW window.
  The code driving this window can be found in the file [FragmentShaderWindow.cpp](src/cpp/FragmentShaderWindow.cpp)

### Building

> [!NOTE]
> This tool is meant to run in a browser and is not a desktop application.
> As a result, it requires [emscripten](https://emscripten.org/) to compile.

Assuming `emscripten` and `CMake` are properly setup and in your path,
this project is straightforward to build:

```text
# to build in Debug mode (use Release to build with optimizations)
> mkdir build
> cd build
> emcmake cmake .. -DCMAKE_BUILD_TYPE=Debug
> cmake --build .
```

### Running

Due to cross-site scripting issues, you cannot load the files directly, but you must serve them instead.
This is a quick and easy way to do this:  

```text
# from the same build folder
> python3 -m http.server 8080
```

Release Notes
-------------

Check the [full history](https://pongasoft.com/webgpu-shader-toy/index.html#release-notes) of release notes.

Misc
----

This project uses the following open source projects (some of them are embedded in this source tree under `external`)

| Project                                                                   | License                                                                             |
|---------------------------------------------------------------------------|-------------------------------------------------------------------------------------|
| [Dear ImGui](https://github.com/ocornut/imgui)                            | [MIT License](https://github.com/ocornut/imgui/blob/master/LICENSE.txt)             |
| [nlohmann/json](https://github.com/nlohmann/json)                         | [MIT License](https://github.com/nlohmann/json/blob/develop/LICENSE.MIT)            |
| [pongasoft/emscripten-glfw](https://github.com/pongasoft/emscripten-glfw) | [Apache 2.0](https://github.com/pongasoft/emscripten-glfw/blob/master/LICENSE.txt)  |
| [ImGuiColorTextEdit](https://github.com/santaclose/ImGuiColorTextEdit)    | [MIT License](https://github.com/santaclose/ImGuiColorTextEdit/blob/master/LICENSE) |

### License

- Apache 2.0 License. This project can be used according to the terms of the Apache 2.0 license.

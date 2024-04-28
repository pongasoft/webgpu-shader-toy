#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <webgpu/webgpu_cpp.h>
#include "Application.h"
#include "MainWindow.h"

std::unique_ptr<shader_toy::Application> kApplication;

static void MainLoopForEmscripten()
{
  if(kApplication && kApplication->running())
    kApplication->mainLoop();
  else
  {
    emscripten_cancel_main_loop();
    kApplication = nullptr;
  }
}

// Main code
int main(int, char **)
{
  IMGUI_CHECKVERSION();

  kApplication = std::make_unique<shader_toy::Application>();

  kApplication
    ->registerRenderable<shader_toy::FragmentShaderWindow>(Renderable::Size{320, 200},
                                                           "WebGPU Shader Toy | fragment shader",
                                                           "#canvas2", "#canvas2-container", "#canvas2-handle")
    ->show();

  kApplication
    ->registerRenderable<shader_toy::MainWindow>(Renderable::Size{320, 200},
                                                 "WebGPU Shader Toy",
                                                 "#canvas1", "#canvas1-container", "#canvas1-handle")
    ->show();

  emscripten_set_main_loop(MainLoopForEmscripten, 0, true);

  return 0;
}

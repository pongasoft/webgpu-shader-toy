#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <webgpu/webgpu_cpp.h>
#include "Application.h"
#include "MainWindow.h"
#include "Model.h"

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
  std::shared_ptr<shader_toy::Model> model{new shader_toy::Model()};

  kApplication = std::make_unique<shader_toy::Application>();

  kApplication
    ->registerRenderable<shader_toy::FragmentShaderWindow>(Window::Args{
                                                             .size = Renderable::Size{320, 200},
                                                             .title = "WebGPU Shader Toy | fragment shader",
                                                             .canvas = {
                                                               .selector = "#canvas2",
                                                               .resizeSelector = "#canvas2-container",
                                                               .handleSelector = "#canvas2-handle"
                                                             }
                                                           },
                                                           model)
    ->show();

  kApplication
    ->registerRenderable<shader_toy::MainWindow>(Window::Args{
                                                   .size = Renderable::Size{320, 200},
                                                   .title = "WebGPU Shader Toy",
                                                   .canvas = {
                                                     .selector = "#canvas1",
                                                     .resizeSelector = "#canvas1-container",
                                                     .handleSelector = "#canvas1-handle"
                                                   }
                                                 },
                                                 model)
    ->show();

  emscripten_set_main_loop(MainLoopForEmscripten, 0, true);

  return 0;
}

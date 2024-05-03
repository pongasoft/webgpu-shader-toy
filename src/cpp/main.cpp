#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "Preferences.h"
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
  kApplication = std::make_unique<shader_toy::Application>();

  std::shared_ptr<shader_toy::Preferences> preferences =
    std::make_unique<shader_toy::Preferences>(std::make_unique<utils::JSStorage>());

  double width2, height2;
  emscripten_get_element_css_size("#canvas2", &width2, &height2);

  double width1, height1;
  emscripten_get_element_css_size("#canvas1", &width1, &height1);

  kApplication
    ->registerRenderable<shader_toy::MainWindow>(Window::Args{
                                                   .size = {static_cast<int>(width1), static_cast<int>(height1)},
                                                   .title = "WebGPU Shader Toy",
                                                   .canvas = {
                                                     .selector = "#canvas1",
                                                     .resizeSelector = "#canvas1-container",
                                                     .handleSelector = "#canvas1-handle"
                                                   }
                                                 },
                                                 shader_toy::MainWindow::Args {
                                                   .fragmentShaderWindow = {
                                                     .size = {static_cast<int>(width2), static_cast<int>(height2)},
                                                     .title = "WebGPU Shader Toy | fragment shader",
                                                     .canvas = {
                                                       .selector = "#canvas2",
                                                       .resizeSelector = "#canvas2-container",
                                                       .handleSelector = "#canvas2-handle"
                                                     }
                                                   },
                                                   .preferences = preferences
                                                 })
    ->show();

  emscripten_set_main_loop(MainLoopForEmscripten, 0, true);

  return 0;
}

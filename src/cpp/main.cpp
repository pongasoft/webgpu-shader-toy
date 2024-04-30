#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "Preferences.h"
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

  std::shared_ptr<shader_toy::Preferences> preferences =
    std::make_unique<shader_toy::Preferences>(std::make_unique<utils::JSStorage>());

  double width, height;
  emscripten_get_element_css_size("#canvas2", &width, &height);

  kApplication
    ->registerRenderable<shader_toy::FragmentShaderWindow>(Window::Args{
                                                             .size = preferences->loadSize(shader_toy::FragmentShaderWindow::kPreferencesSizeKey,
                                                                                           {static_cast<int>(width), static_cast<int>(height)}),
                                                             .title = "WebGPU Shader Toy | fragment shader",
                                                             .canvas = {
                                                               .selector = "#canvas2",
                                                               .resizeSelector = "#canvas2-container",
                                                               .handleSelector = "#canvas2-handle"
                                                             }
                                                           },
                                                           shader_toy::FragmentShaderWindow::Args {
                                                             .model = model,
                                                             .preferences = preferences
                                                           })
    ->show();

  emscripten_get_element_css_size("#canvas1", &width, &height);

  kApplication
    ->registerRenderable<shader_toy::MainWindow>(Window::Args{
                                                   .size = preferences->loadSize(shader_toy::MainWindow::kPreferencesSizeKey,
                                                                                 {static_cast<int>(width), static_cast<int>(height)}),
                                                   .title = "WebGPU Shader Toy",
                                                   .canvas = {
                                                     .selector = "#canvas1",
                                                     .resizeSelector = "#canvas1-container",
                                                     .handleSelector = "#canvas1-handle"
                                                   }
                                                 },
                                                 shader_toy::MainWindow::Args {
                                                   .model = model,
                                                   .preferences = preferences
                                                 })
    ->show();

  emscripten_set_main_loop(MainLoopForEmscripten, 0, true);

  return 0;
}

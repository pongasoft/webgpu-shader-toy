#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <webgpu/webgpu_cpp.h>
#include "Application.h"

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
  kApplication->init("#canvas");

  emscripten_set_main_loop(MainLoopForEmscripten, 0, true);

  return 0;
}

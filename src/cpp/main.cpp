#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "Preferences.h"
#include "Application.h"
#include "MainWindow.h"
#include "State.h"

std::unique_ptr<shader_toy::Application> kApplication;

/**
 * Main loop for emscripten */
static void MainLoopForEmscripten()
{
  try
  {
    if(kApplication && kApplication->running())
      kApplication->mainLoop();
    else
    {
      emscripten_cancel_main_loop();
      kApplication = nullptr;
    }
  }
  catch(std::exception &e)
  {
    printf("ABORT| Unrecoverable exception detected: %s\n", e.what());
    abort();
  }
  catch(...)
  {
    printf("ABORT| Unrecoverable exception detected: Unknown exception\n");
    abort();
  }
}

//! wstDoneWaiting()
EM_JS(bool, wstDoneWaiting, (), { return Module.wst_done_waiting; })

//! wstWaitForContinue()
EM_JS(bool, wstWaitForContinue, (bool iManualLayout, int iLeftPaneSize, int iRightPaneSize), {
  Module.wst_wait_for_continue(iManualLayout, iLeftPaneSize, iRightPaneSize);
})

/**
 * Wait loop: wait for the user to click continue to swap emscripten main loop
 */
static void WaitLoopForEmscripten()
{
  if(wstDoneWaiting())
  {
    emscripten_cancel_main_loop();
    emscripten_set_main_loop(MainLoopForEmscripten, 0, true);
  }
}

namespace shader_toy {
extern std::vector<Shader> kBuiltInFragmentShaderExamples;
}

shader_toy::State computeDefaultState()
{
  double width2, height2;
  emscripten_get_element_css_size("#canvas2", &width2, &height2);

  double width1, height1;
  emscripten_get_element_css_size("#canvas1", &width1, &height1);

  shader_toy::State state{};
  state.fSettings.fMainWindowSize = {static_cast<int>(width1), static_cast<int>(height1)};
  state.fSettings.fFragmentShaderWindowSize = {static_cast<int>(width2), static_cast<int>(height2)};

  auto defaultShader = shader_toy::kBuiltInFragmentShaderExamples[0];
  defaultShader.fWindowSize = state.fSettings.fFragmentShaderWindowSize;
  state.fShaders.fList.emplace_back(defaultShader);
  state.fShaders.fCurrent = defaultShader.fName;

  return state;
}

// Main code
int main(int, char **)
{
  try
  {
    kApplication = std::make_unique<shader_toy::Application>();

    std::shared_ptr<shader_toy::Preferences> preferences =
      std::make_unique<shader_toy::Preferences>(std::make_unique<utils::JSStorage>());

    auto defaultState = computeDefaultState();
    auto state = preferences->loadState(shader_toy::Preferences::kStateKey, defaultState);
    defaultState.fShaders.fList.clear();
    defaultState.fShaders.fCurrent = std::nullopt;

    kApplication
      ->registerRenderable<shader_toy::MainWindow>(Window::Args{
                                                     .size = state.fSettings.fMainWindowSize,
                                                     .title = "WebGPU Shader Toy",
                                                     .canvas = { .selector = "#canvas1" }
                                                   },
                                                   shader_toy::MainWindow::Args {
                                                     .fragmentShaderWindow = {
                                                       .size = state.fSettings.fFragmentShaderWindowSize,
                                                       .title = "WebGPU Shader Toy",
                                                       .canvas = { .selector = "#canvas2" }
                                                     },
                                                     .defaultState = defaultState,
                                                     .state = state,
                                                     .preferences = preferences
                                                   })
      ->show();

#ifndef NDEBUG
    emscripten::glfw3::AddBrowserKeyCallback([](GLFWwindow* window, int key, int scancode, int action, int mods) {
      return mods == 0 && action == GLFW_PRESS && key == GLFW_KEY_F12;
    });
#endif

    wstWaitForContinue(state.fSettings.fLayoutManual, state.fSettings.fMainWindowSize.width, state.fSettings.fFragmentShaderWindowSize.width);

    emscripten_set_main_loop(WaitLoopForEmscripten, 0, true);
  }
  catch(std::exception &e)
  {
    printf("ABORT| Unrecoverable exception detected: %s\n", e.what());
    abort();
  }
  catch(...)
  {
    printf("ABORT| Unrecoverable exception detected: Unknown exception\n");
    abort();
  }

  return 0;
}

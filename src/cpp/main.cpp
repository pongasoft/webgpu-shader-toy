#include <backends/imgui_impl_glfw.h>
#include <emscripten.h>
#include <emscripten/html5.h>
#include "Preferences.h"
#include "Application.h"
#include "MainWindow.h"
#include "State.h"

using MaybeApplication = std::future<std::unique_ptr<shader_toy::Application>>;

MaybeApplication kApplicationFuture{};
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
EM_JS(bool, wstDoneWaiting, (), { return Module['wst_done_waiting']; })

//! wstWaitForContinue()
EM_JS(bool, wstWaitForContinue, (), { Module['wst_wait_for_continue'](); })

//! wstShowError(message)
EM_JS(void, wstShowError, (char const *iMessage), {
  Module['wst_show_error']('<h3>' +
    UTF8ToString(iMessage) +
    '<br>WebGPU might not be supported by this browser' +
    '<br>Try with Google Chrome or Microsoft Edge' +
    '<br>Check <a href="https://caniuse.com/webgpu">this site</a> for details' +
    '</h3>');
})

/**
 * Cancel the current main loop and set a new one */
static void emscripten_update_main_loop(em_callback_func func, int fps, bool simulate_infinite_loop)
{
  emscripten_cancel_main_loop();
  emscripten_set_main_loop(func, fps, simulate_infinite_loop);
}


/**
 * Wait loop: wait for the user to click continue to swap emscripten main loop
 */
static void WaitLoopUserClickContinue()
{
  if(wstDoneWaiting())
  {
    emscripten_update_main_loop(MainLoopForEmscripten, 0, true);
  }
}

namespace shader_toy {

extern std::vector<Shader> kBuiltInFragmentShaderExamples;
}

/**
 * Computes the default state of the application before applying user preferences */
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

/**
 * The first phase is to wait for the application to be created: we check the future every time
 * Emscripten invokes the main loop.
 */
static void WaitLoopForApplication()
{
  if(kApplicationFuture.wait_for(std::chrono::milliseconds(5)) == std::future_status::ready)
  {
    // The application was created => we can proceed
    kApplication = kApplicationFuture.get();
    kApplicationFuture = {};

    try
    {
      std::shared_ptr preferences =
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

    // We notify the page that initialization is complete, and it's time to display the "Continue" button
    wstWaitForContinue();

    emscripten_update_main_loop(WaitLoopUserClickContinue, 0, true);
  }
}

// Main code
int main(int, char **)
{
  kApplicationFuture = shader_toy::Application::asyncCreate([](std::string_view message) {
    // Error while creating the application...

    // cancel main loop
    emscripten_cancel_main_loop();

    // render error message
    char errorMessage[1024];
    snprintf(errorMessage, 1024, "%.*s\n", static_cast<int>(message.length()), message.data());
    wstShowError(errorMessage);
  });

  emscripten_set_main_loop(WaitLoopForApplication, 0, true);

  return 0;
}

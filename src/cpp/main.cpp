#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_wgpu.h>
#include <cstdio>
#include <emscripten.h>
#include <emscripten/html5_webgpu.h>
#include <GLFW/emscripten_glfw3.h>
#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <webgpu/webgpu_cpp.h>

// Global WebGPU required states
static WGPUDevice wgpu_device = nullptr;
static WGPUSurface wgpu_surface = nullptr;
static WGPUTextureFormat wgpu_preferred_fmt = WGPUTextureFormat_RGBA8Unorm;
static WGPUSwapChain wgpu_swap_chain = nullptr;
static int wgpu_swap_chain_width = 0;
static int wgpu_swap_chain_height = 0;

// Forward declarations
static bool InitWGPU();

static void CreateSwapChain(int width, int height);

static void glfw_error_callback(int error, const char *description)
{
  printf("GLFW Error %d: %s\n", error, description);
}

static void wgpu_error_callback(WGPUErrorType error_type, const char *message, void *)
{
  const char *error_type_lbl = "";
  switch(error_type)
  {
    case WGPUErrorType_Validation:
      error_type_lbl = "Validation";
      break;
    case WGPUErrorType_OutOfMemory:
      error_type_lbl = "Out of memory";
      break;
    case WGPUErrorType_Unknown:
      error_type_lbl = "Unknown";
      break;
    case WGPUErrorType_DeviceLost:
      error_type_lbl = "Device lost";
      break;
    default:
      error_type_lbl = "Unknown";
  }
  printf("%s error: %s\n", error_type_lbl, message);
}

struct App
{
  std::function<bool()> renderFrame{};
  std::function<void()> cleanup{};
};

static void MainLoopForEmscripten(void *iUserData)
{
  auto app = reinterpret_cast<App *>(iUserData);
  if(app->renderFrame())
  {
    if(app->cleanup)
      app->cleanup();
    emscripten_cancel_main_loop();
  }
}

// Main code
int main(int, char **)
{
  glfwSetErrorCallback(glfw_error_callback);
  printf("%s\n", glfwGetVersionString());
  if(!glfwInit())
    return 1;

  // Make sure GLFW does not initialize any graphics context.
  // This needs to be done explicitly later.
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  emscripten_glfw_set_next_window_canvas_selector("#canvas");
  GLFWwindow *window = glfwCreateWindow(1280, 720, "Dear ImGui GLFW+WebGPU example", nullptr, nullptr);
  if(window == nullptr)
    return 1;

  // Initialize the WebGPU environment
  if(!InitWGPU())
  {
    if(window)
      glfwDestroyWindow(window);
    glfwTerminate();
    return 1;
  }
  glfwShowWindow(window);

  // Setup Dear ImGui context
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void) io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

#ifdef IMGUI_ENABLE_DOCKING
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigDockingWithShift = false;
#endif

  // Setup Dear ImGui style
  ImGui::StyleColorsDark();
  //ImGui::StyleColorsLight();

  // Setup Platform/Renderer backends
  ImGui_ImplGlfw_InitForOther(window, true);
  // makes the canvas resizable and match the full window size
  emscripten_glfw_make_canvas_resizable(window, "window", nullptr);
  ImGui_ImplWGPU_InitInfo init_info;
  init_info.Device = wgpu_device;
  init_info.NumFramesInFlight = 3;
  init_info.RenderTargetFormat = wgpu_preferred_fmt;
  init_info.DepthStencilFormat = WGPUTextureFormat_Undefined;
  ImGui_ImplWGPU_Init(&init_info);

  // Our state
  bool show_demo_window = true;
  bool show_another_window = false;
  ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

  // Main loop
  App app{};
  app.renderFrame = [&]() {
    // Poll and handle events (inputs, window resize, etc.)
    // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
    // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
    // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
    // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
    glfwPollEvents();

    // React to changes in screen size
    int width, height;
    glfwGetFramebufferSize((GLFWwindow *) window, &width, &height);
    if(width != wgpu_swap_chain_width && height != wgpu_swap_chain_height)
    {
      ImGui_ImplWGPU_InvalidateDeviceObjects();
      CreateSwapChain(width, height);
      ImGui_ImplWGPU_CreateDeviceObjects();
    }

    // Start the Dear ImGui frame
    ImGui_ImplWGPU_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

#ifdef IMGUI_ENABLE_DOCKING
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport());
#endif

#ifndef IMGUI_DISABLE_DEMO
    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if(show_demo_window)
      ImGui::ShowDemoWindow(&show_demo_window);
#endif

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    {
      static float f = 0.0f;
      static int counter = 0;

      ImGui::Begin(
        "Hello, world!");                                // Create a window called "Hello, world!" and append into it.

      ImGui::Text(
        "This is some useful text.");                     // Display some text (you can use a format strings too)
      ImGui::Checkbox("Demo Window", &show_demo_window);            // Edit bools storing our window open/close state
      ImGui::Checkbox("Another Window", &show_another_window);

      ImGui::SliderFloat("float", &f, 0.0f, 1.0f);                  // Edit 1 float using a slider from 0.0f to 1.0f
      ImGui::ColorEdit3("clear color", (float *) &clear_color);       // Edit 3 floats representing a color

      if(ImGui::Button(
        "Button"))                                  // Buttons return true when clicked (most widgets return true when edited/activated)
        counter++;
      ImGui::SameLine();
      ImGui::Text("counter = %d", counter);

      if(ImGui::Button("Exit"))
        glfwSetWindowShouldClose(window, GLFW_TRUE);

      ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
      ImGui::End();
    }

    // 3. Show another simple window.
    if(show_another_window)
    {
      ImGui::Begin("Another Window",
                   &show_another_window);         // Pass a pointer to our bool variable (the window will have a closing button that will clear the bool when clicked)
      ImGui::Text("Hello from another window!");
      if(ImGui::Button("Close Me"))
        show_another_window = false;
      ImGui::End();
    }

    // Rendering
    ImGui::Render();

    WGPURenderPassColorAttachment color_attachments = {};
    color_attachments.depthSlice = WGPU_DEPTH_SLICE_UNDEFINED;
    color_attachments.loadOp = WGPULoadOp_Clear;
    color_attachments.storeOp = WGPUStoreOp_Store;
    color_attachments.clearValue = {clear_color.x * clear_color.w, clear_color.y * clear_color.w,
                                    clear_color.z * clear_color.w, clear_color.w};
    color_attachments.view = wgpuSwapChainGetCurrentTextureView(wgpu_swap_chain);

    WGPURenderPassDescriptor render_pass_desc = {};
    render_pass_desc.colorAttachmentCount = 1;
    render_pass_desc.colorAttachments = &color_attachments;
    render_pass_desc.depthStencilAttachment = nullptr;

    WGPUCommandEncoderDescriptor enc_desc = {};
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(wgpu_device, &enc_desc);

    WGPURenderPassEncoder pass = wgpuCommandEncoderBeginRenderPass(encoder, &render_pass_desc);
    ImGui_ImplWGPU_RenderDrawData(ImGui::GetDrawData(), pass);
    wgpuRenderPassEncoderEnd(pass);

    WGPUCommandBufferDescriptor cmd_buffer_desc = {};
    WGPUCommandBuffer cmd_buffer = wgpuCommandEncoderFinish(encoder, &cmd_buffer_desc);
    WGPUQueue queue = wgpuDeviceGetQueue(wgpu_device);
    wgpuQueueSubmit(queue, 1, &cmd_buffer);
    return glfwWindowShouldClose(window);
  };

  app.cleanup = [window]() {
    ImGui_ImplWGPU_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
  };

  emscripten_set_main_loop_arg(MainLoopForEmscripten, &app, 0, true);

  return 0;
}

static bool InitWGPU()
{
  wgpu_device = emscripten_webgpu_get_device();
  if(!wgpu_device)
    return false;

  wgpuDeviceSetUncapturedErrorCallback(wgpu_device, wgpu_error_callback, nullptr);

  // Use C++ wrapper due to misbehavior in Emscripten.
  // Some offset computation for wgpuInstanceCreateSurface in JavaScript
  // seem to be inline with struct alignments in the C++ structure
  wgpu::SurfaceDescriptorFromCanvasHTMLSelector html_surface_desc = {};
  html_surface_desc.selector = "#canvas";

  wgpu::SurfaceDescriptor surface_desc = {};
  surface_desc.nextInChain = &html_surface_desc;

  wgpu::Instance instance = wgpuCreateInstance(nullptr);
  wgpu::Surface surface = instance.CreateSurface(&surface_desc);
  wgpu::Adapter adapter = {};
  wgpu_preferred_fmt = (WGPUTextureFormat) surface.GetPreferredFormat(adapter);
  wgpu_surface = surface.MoveToCHandle();

  return true;
}

static void CreateSwapChain(int width, int height)
{
  if(wgpu_swap_chain)
    wgpuSwapChainRelease(wgpu_swap_chain);
  wgpu_swap_chain_width = width;
  wgpu_swap_chain_height = height;
  WGPUSwapChainDescriptor swap_chain_desc = {};
  swap_chain_desc.usage = WGPUTextureUsage_RenderAttachment;
  swap_chain_desc.format = wgpu_preferred_fmt;
  swap_chain_desc.width = width;
  swap_chain_desc.height = height;
  swap_chain_desc.presentMode = WGPUPresentMode_Fifo;
  wgpu_swap_chain = wgpuDeviceCreateSwapChain(wgpu_device, wgpu_surface, &swap_chain_desc);
}

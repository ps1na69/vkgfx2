#include "vkgfx2/Renderer.h"

#include <spdlog/spdlog.h>

namespace {

constexpr SDL_GPUShaderFormat kSupportedShaderFormats =
    SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV;

#if defined(NDEBUG)
constexpr bool kEnableGpuDebugLayer = false;
#else
constexpr bool kEnableGpuDebugLayer = true;
#endif

} // namespace

Renderer::~Renderer()
{
    if (device_ == nullptr) {
        return;
    }

    SDL_WaitForGPUIdle(device_);
    SDL_ReleaseWindowFromGPUDevice(device_, window_);
    SDL_DestroyGPUDevice(device_);
}

bool Renderer::initialize(SDL_Window* window)
{
    window_ = window;

    device_ = SDL_CreateGPUDevice(kSupportedShaderFormats, kEnableGpuDebugLayer, nullptr);
    if (device_ == nullptr) {
        spdlog::error("Could not create SDL GPU device: {}", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device_, window_)) {
        spdlog::error("Could not create swapchain: {}", SDL_GetError());
        SDL_DestroyGPUDevice(device_);
        device_ = nullptr;
        return false;
    }

    spdlog::info("SDL GPU renderer initialized with '{}' backend", SDL_GetGPUDeviceDriver(device_));
    return true;
}

bool Renderer::renderFrame()
{
    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(device_);
    if (commandBuffer == nullptr) {
        spdlog::error("Could not acquire GPU command buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUTexture* swapchainTexture = nullptr;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            commandBuffer, window_, &swapchainTexture, nullptr, nullptr)) {
        spdlog::error("Could not acquire swapchain texture: {}", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return false;
    }

    // A minimized window has no drawable swapchain image. There is no work to submit.
    if (swapchainTexture == nullptr) {
        SDL_CancelGPUCommandBuffer(commandBuffer);
        return true;
    }

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchainTexture;
    colorTarget.clear_color = { 0.02f, 0.03f, 0.06f, 1.0f };
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        commandBuffer, &colorTarget, 1, nullptr);
    SDL_EndGPURenderPass(renderPass);

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        spdlog::error("Could not submit GPU command buffer: {}", SDL_GetError());
        return false;
    }

    return true;
}

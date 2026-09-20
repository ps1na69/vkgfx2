#include "vkgfx2/Renderer.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <vector>

namespace {

constexpr SDL_GPUShaderFormat kSupportedShaderFormats =
    SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV;

#if defined(NDEBUG)
constexpr bool kEnableGpuDebugLayer = false;
#else
constexpr bool kEnableGpuDebugLayer = true;
#endif

std::vector<Uint8> readBinaryFile(const std::filesystem::path& path)
{
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        spdlog::error("Could not open shader: {}", path.string());
        return {};
    }

    const auto size = file.tellg();
    std::vector<Uint8> bytes(static_cast<size_t>(size));
    file.seekg(0);
    file.read(reinterpret_cast<char*>(bytes.data()), size);
    return bytes;
}

SDL_GPUShader* loadShader(
    SDL_GPUDevice* device,
    const std::filesystem::path& path,
    SDL_GPUShaderStage stage)
{
    const auto code = readBinaryFile(path);
    if (code.empty()) {
        return nullptr;
    }

    SDL_GPUShaderCreateInfo createInfo{};
    createInfo.code = code.data();
    createInfo.code_size = code.size();
    createInfo.entrypoint = "main";
    createInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
    createInfo.stage = stage;
    return SDL_CreateGPUShader(device, &createInfo);
}

} // namespace

Renderer::~Renderer()
{
    if (device_ == nullptr) {
        return;
    }

    SDL_WaitForGPUIdle(device_);
    if (pipeline_ != nullptr) {
        SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
    }
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

    const std::filesystem::path shaderDirectory =
        std::filesystem::path(SDL_GetBasePath()) / "shaders";
    SDL_GPUShader* vertexShader = loadShader(
        device_, shaderDirectory / "triangle.vert.dxil", SDL_GPU_SHADERSTAGE_VERTEX);
    SDL_GPUShader* fragmentShader = loadShader(
        device_, shaderDirectory / "triangle.frag.dxil", SDL_GPU_SHADERSTAGE_FRAGMENT);

    if (vertexShader == nullptr || fragmentShader == nullptr) {
        spdlog::error("Could not create triangle shaders: {}", SDL_GetError());
        if (vertexShader != nullptr) {
            SDL_ReleaseGPUShader(device_, vertexShader);
        }
        if (fragmentShader != nullptr) {
            SDL_ReleaseGPUShader(device_, fragmentShader);
        }
        return false;
    }

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);
    colorTarget.blend_state.color_write_mask =
        SDL_GPU_COLORCOMPONENT_R | SDL_GPU_COLORCOMPONENT_G |
        SDL_GPU_COLORCOMPONENT_B | SDL_GPU_COLORCOMPONENT_A;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.enable_depth_clip = true;
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pipelineInfo);

    SDL_ReleaseGPUShader(device_, vertexShader);
    SDL_ReleaseGPUShader(device_, fragmentShader);

    if (pipeline_ == nullptr) {
        spdlog::error("Could not create triangle pipeline: {}", SDL_GetError());
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
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline_);
    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
    SDL_EndGPURenderPass(renderPass);

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        spdlog::error("Could not submit GPU command buffer: {}", SDL_GetError());
        return false;
    }

    return true;
}

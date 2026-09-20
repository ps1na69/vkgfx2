#include "vkgfx2/Renderer.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <array>
#include <cstddef>
#include <cstring>
#include <vector>

namespace {

constexpr SDL_GPUShaderFormat kSupportedShaderFormats =
    SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_SPIRV;

struct Vertex {
    float position[2];
    float color[3];
};

// Three separated golden triangles form a Triforce-like silhouette.
constexpr std::array<Vertex, 6> kTriangleVertices = {{
    {{ 0.00f, -0.78f}, {1.00f, 0.80f, 0.16f}}, // Top
    {{-0.72f,  0.62f}, {0.95f, 0.62f, 0.08f}}, // Bottom-left
    {{ 0.72f,  0.62f}, {1.00f, 0.88f, 0.28f}}, // Bottom-right
    {{-0.36f, -0.08f}, {1.00f, 0.92f, 0.38f}}, // Inner-left
    {{ 0.36f, -0.08f}, {1.00f, 0.92f, 0.38f}}, // Inner-right
    {{ 0.00f,  0.62f}, {1.00f, 0.74f, 0.12f}}, // Inner-bottom
}};

constexpr std::array<Uint16, 9> kTriangleIndices = {{
    0, 3, 4, // Top triangle
    3, 1, 5, // Bottom-left triangle
    4, 5, 2, // Bottom-right triangle
}};

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
    createInfo.num_uniform_buffers = stage == SDL_GPU_SHADERSTAGE_VERTEX ? 1 : 0;
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
    if (vertexBuffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, vertexBuffer_);
    }
    if (indexBuffer_ != nullptr) {
        SDL_ReleaseGPUBuffer(device_, indexBuffer_);
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

    SDL_GPUBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    vertexBufferInfo.size = sizeof(kTriangleVertices);
    vertexBuffer_ = SDL_CreateGPUBuffer(device_, &vertexBufferInfo);
    if (vertexBuffer_ == nullptr) {
        spdlog::error("Could not create vertex buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
    indexBufferInfo.size = sizeof(kTriangleIndices);
    indexBuffer_ = SDL_CreateGPUBuffer(device_, &indexBufferInfo);
    if (indexBuffer_ == nullptr) {
        spdlog::error("Could not create index buffer: {}", SDL_GetError());
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferBufferInfo{};
    transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferBufferInfo.size = sizeof(kTriangleVertices) + sizeof(kTriangleIndices);
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(
        device_, &transferBufferInfo);
    if (transferBuffer == nullptr) {
        spdlog::error("Could not create vertex transfer buffer: {}", SDL_GetError());
        return false;
    }

    void* mappedMemory = SDL_MapGPUTransferBuffer(device_, transferBuffer, false);
    if (mappedMemory == nullptr) {
        spdlog::error("Could not map vertex transfer buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device_, transferBuffer);
        return false;
    }
    std::memcpy(mappedMemory, kTriangleVertices.data(), sizeof(kTriangleVertices));
    std::memcpy(
        static_cast<std::byte*>(mappedMemory) + sizeof(kTriangleVertices),
        kTriangleIndices.data(),
        sizeof(kTriangleIndices));
    SDL_UnmapGPUTransferBuffer(device_, transferBuffer);

    SDL_GPUCommandBuffer* uploadCommandBuffer = SDL_AcquireGPUCommandBuffer(device_);
    if (uploadCommandBuffer == nullptr) {
        spdlog::error("Could not acquire vertex upload command buffer: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device_, transferBuffer);
        return false;
    }

    SDL_GPUTransferBufferLocation source{};
    source.transfer_buffer = transferBuffer;
    SDL_GPUBufferRegion destination{};
    destination.buffer = vertexBuffer_;
    destination.size = sizeof(kTriangleVertices);

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(uploadCommandBuffer);
    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);

    source.offset = sizeof(kTriangleVertices);
    destination.buffer = indexBuffer_;
    destination.size = sizeof(kTriangleIndices);
    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);
    SDL_EndGPUCopyPass(copyPass);

    if (!SDL_SubmitGPUCommandBuffer(uploadCommandBuffer)) {
        spdlog::error("Could not submit vertex upload: {}", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device_, transferBuffer);
        return false;
    }
    SDL_ReleaseGPUTransferBuffer(device_, transferBuffer);

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

    SDL_GPUVertexBufferDescription vertexBufferDescription{};
    vertexBufferDescription.slot = 0;
    vertexBufferDescription.pitch = sizeof(Vertex);
    vertexBufferDescription.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;

    std::array<SDL_GPUVertexAttribute, 2> vertexAttributes{};
    vertexAttributes[0].location = 0;
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[0].offset = offsetof(Vertex, position);
    vertexAttributes[1].location = 1;
    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[1].offset = offsetof(Vertex, color);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBufferDescription;
    pipelineInfo.vertex_input_state.num_vertex_attributes = vertexAttributes.size();
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes.data();
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

bool Renderer::renderFrame(const std::array<float, 16>& transformMatrix)
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

    SDL_PushGPUVertexUniformData(
        commandBuffer, 0, transformMatrix.data(), sizeof(transformMatrix));

    SDL_GPURenderPass* renderPass = SDL_BeginGPURenderPass(
        commandBuffer, &colorTarget, 1, nullptr);
    SDL_BindGPUGraphicsPipeline(renderPass, pipeline_);
    SDL_GPUBufferBinding vertexBufferBinding{};
    vertexBufferBinding.buffer = vertexBuffer_;
    SDL_BindGPUVertexBuffers(renderPass, 0, &vertexBufferBinding, 1);
    SDL_GPUBufferBinding indexBufferBinding{};
    indexBufferBinding.buffer = indexBuffer_;
    SDL_BindGPUIndexBuffer(renderPass, &indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
    SDL_DrawGPUIndexedPrimitives(renderPass, kTriangleIndices.size(), 1, 0, 0, 0);
    SDL_EndGPURenderPass(renderPass);

    if (!SDL_SubmitGPUCommandBuffer(commandBuffer)) {
        spdlog::error("Could not submit GPU command buffer: {}", SDL_GetError());
        return false;
    }

    return true;
}

#pragma once

#include <SDL3/SDL.h>

#include <array>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    [[nodiscard]] bool initialize(SDL_Window* window);
    [[nodiscard]] bool renderFrame(const std::array<float, 16>& transformMatrix);

private:
    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUBuffer* vertexBuffer_ = nullptr;
    SDL_GPUBuffer* indexBuffer_ = nullptr;
};

#pragma once

#include <SDL3/SDL.h>

class Renderer {
public:
    Renderer() = default;
    ~Renderer();

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;

    [[nodiscard]] bool initialize(SDL_Window* window);
    [[nodiscard]] bool renderFrame();

private:
    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* device_ = nullptr;
};

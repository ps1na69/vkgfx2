#pragma once
#include <SDL3/SDL.h>
#include <string_view>

class Window {
public:
    Window(std::string_view title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    [[nodiscard]] SDL_Window* nativeHandle() const noexcept;
    [[nodiscard]] int width() const noexcept;
    [[nodiscard]] int height() const noexcept;

    void handleEvent(const SDL_Event& event);

private:
    SDL_Window* handle_ = nullptr;
    int width_ = 0;
    int height_ = 0;
};
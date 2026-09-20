#include "vkgfx2/Window.h"

#include <stdexcept>
#include <string>

Window::Window(std::string_view title, int width, int height)
    : width_(width), height_(height)
{
    const std::string windowTitle(title);

    handle_ = SDL_CreateWindow(
        windowTitle.c_str(),
        width_,
        height_,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (handle_ == nullptr) {
        throw std::runtime_error(
            std::string("Window creation failed: ") + SDL_GetError()
        );
    }
}

Window::~Window()
{
    if (handle_ != nullptr) {
        SDL_DestroyWindow(handle_);
    }
}

SDL_Window* Window::nativeHandle() const noexcept
{
    return handle_;
}

int Window::width() const noexcept
{
    return width_;
}

int Window::height() const noexcept
{
    return height_;
}

void Window::handleEvent(const SDL_Event& event)
{
    if (event.type != SDL_EVENT_WINDOW_RESIZED) {
        return;
    }

    if (event.window.windowID != SDL_GetWindowID(handle_)) {
        return;
    }

    width_ = event.window.data1;
    height_ = event.window.data2;
}
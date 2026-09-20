#include "vkgfx2/Application.h"
#include "vkgfx2/Window.h"

#include <SDL3/SDL.h>

#include <memory>

Application::~Application()
{
    window_.reset();

    SDL_Quit();
}

bool Application::initialize()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return false;
    }

    window_ = std::make_unique<Window>(
        "vkgfx2",
        1280,
        720
    );

    running_ = true;
    return true;
}

int Application::run()
{
    while (running_) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running_ = false;
                continue;
            }

            window_->handleEvent(event);
        }

        SDL_Delay(1);
    }

    return 0;
}
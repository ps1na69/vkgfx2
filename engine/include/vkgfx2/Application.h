#pragma once

#include <memory>
#include "Renderer.h"
#include "Transform.h"
#include "Window.h"

class Application {
public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool initialize();
    int run();

private:
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer> renderer_;
    Transform transform_;
    Uint64 previousFrameTime_ = 0;
    bool running_ = false;
};

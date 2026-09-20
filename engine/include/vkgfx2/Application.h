#pragma once

#include <memory>
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
    bool running_ = false;
};
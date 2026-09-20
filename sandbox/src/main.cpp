#include "vkgfx2/Application.h"

#include <exception>

int main()
{
    try {
        Application application;

        if (!application.initialize()) {
            return 1;
        }

        return application.run();
    }
    catch (const std::exception&) {
        return 1;
    }
}
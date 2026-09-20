#pragma once

#include <array>

class Transform {
public:
    void rotate(float radians) noexcept;

    [[nodiscard]] std::array<float, 16> matrix() const noexcept;

private:
    float rotationRadians_ = 0.0f;
};

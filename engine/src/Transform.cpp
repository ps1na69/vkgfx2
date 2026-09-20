#include "vkgfx2/Transform.h"

#include <cmath>

void Transform::rotate(float radians) noexcept
{
    rotationRadians_ += radians;
}

std::array<float, 16> Transform::matrix() const noexcept
{
    const float cosine = std::cos(rotationRadians_);
    const float sine = std::sin(rotationRadians_);

    // Row-major 4x4 matrix. The shader declares the matching row_major layout.
    return {
        cosine, -sine, 0.0f, 0.0f,
        sine,    cosine, 0.0f, 0.0f,
        0.0f,    0.0f,  1.0f, 0.0f,
        0.0f,    0.0f,  0.0f, 1.0f,
    };
}

#include <cmath>
#include <iostream>

#include "educelab/core/types/Mat.hpp"
#include "educelab/core/types/Vec.hpp"
#include "educelab/core/utils/Iteration.hpp"
#include "educelab/core/utils/Math.hpp"

using namespace educelab;

auto main() -> int
{
    // Type aliases
    using Matrix = Mat<4, 4, float>;
    using Point = Vec<float, 4>;

    // Input point
    Point p{0, 0, 0, 1};
    std::cout << "Starting point: " << p << "\n\n";

    // Translate 1 unit along x
    auto translate = Matrix::Eye();
    translate(0, 3) = 1.f;
    p = translate * p;
    std::cout << "Translation matrix:\n" << translate << "\n";
    std::cout << "After translation: " << p << "\n\n";

    // Rotate around z
    auto theta = to_radians(90.F);
    auto rotate = Matrix::Eye();
    rotate(0, 0) = std::cos(theta);
    rotate(0, 1) = -std::sin(theta);
    rotate(1, 0) = std::sin(theta);
    rotate(1, 1) = std::cos(theta);
    p = rotate * p;
    std::cout << "Rotation matrix:\n" << rotate << "\n";
    std::cout << "After rotation: " << p << "\n\n";

    // Scale by 10
    auto scale = Matrix::Eye();
    for (auto i : range(3)) {
        scale(i, i) = 10.F;
    }
    p = scale * p;
    std::cout << "Scale matrix:\n" << scale << "\n";
    std::cout << "After scale: " << p << "\n\n";

    // Restore original point and apply a single multiplication
    p = {0, 0, 0, 1};
    auto transform = scale * rotate * translate;
    std::cout << "Restored starting point: " << p << "\n";
    p = transform * p;
    std::cout << "Transform matrix:\n" << transform << "\n";
    std::cout << "After transform: " << p << "\n";
}
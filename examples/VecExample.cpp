#include <iostream>
#include <vector>

#include "educelab/core/types/Vec.hpp"

using namespace educelab;

auto main() -> int
{
    // Default constructor
    Vec3f v0;
    std::cout << Vec3f() << std::endl;  // [0, 0, 0]

    // With initial values
    Vec3f v1{0, 1, 0};

    // Assignment from STL-like containers
    v0 = {1, 0, 0};
    v1 = std::vector<int>{0, 1, 0};

    // Basic arithmetic
    std::cout << v0 + v1 << "\n";  // [1, 1, 0]
    std::cout << v0 - v1 << "\n";  // [1, -1, 0]
    std::cout << v0 * 5 << "\n";   // [5, 0, 0]
    std::cout << v0 / 5 << "\n";   // [0.2, 0, 0]

    // Normalization and length
    auto v2 = v0 * 5;
    std::cout << v2.magnitude() << "\n";  // 5
    std::cout << v2.unit() << "\n";       // [1, 0, 0]

    // Dot (inner) and cross product
    std::cout << v0.cross(v1) << "\n";  // [0, 0, 1]
    std::cout << v0.dot(v1) << "\n";    // 0

    // Templated N-dim vectors
    using Vec5f = Vec<float, 5>;
    Vec5f v3{1, 2, 3, 4, 5};
    Vec5f v4{5, 4, 3, 2, 1};
    std::cout << v3 * 1.5F << "\n";   // [1.5, 3, 4.5, 6, 7.5]
    std::cout << v3 / 5.F << "\n";    // [0.2, 0.4, 0.6, 0.8, 1]
    std::cout << v3.dot(v4) << "\n";  // 35
}
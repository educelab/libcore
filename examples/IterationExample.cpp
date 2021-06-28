#include <iostream>
#include <vector>

#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

int main()
{
    // 1D numerical ranges
    for (const auto& i : range(5)) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    // 0 1 2 3 4

    // 1D numerical ranges with start, stop, step
    for (const auto& i : range(4, 12, 2)) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    // 4 6 8 10

    // Floating-point numerical ranges
    for (const auto& i : range(0.f, 1.f, 0.25f)) {
        std::cout << i << " ";
    }
    std::cout << "\n";
    // 0 0.25 0.5 0.75

    // 2D numerical ranges
    for (const auto& [y, x] : range2D(3, 3)) {
        std::cout << "(" << x << ", " << y << ") ";
    }
    std::cout << "\n";
    // [0, 0] [1, 0] [2, 0] [0, 1] [1, 1] [2, 1] [0, 2] [1, 2] [2, 2]

    // Enumerate containers
    std::vector<std::string> values{"The", "quick,", "brown", "fox."};
    for (const auto& [idx, val] : enumerate(values)) {
        std::cout << "[" << idx << "] " << val << " ";
    }
    std::cout << "\n";
    // [0] The [1] quick, [2] brown [3] fox.

    // Enumerate parameter packs
    for (const auto& [idx, val] : enumerate("The", "quick,", "brown", "fox.")) {
        std::cout << "[" << idx << "] " << val << " ";
    }
    std::cout << "\n";
    // [0] The [1] quick, [2] brown [3] fox.

    // Enumerate ranges
    for (auto it : enumerate(range2D(2, 2))) {
        const auto idx = it.first;
        const auto [y, x] = it.second;
        std::cout << "[" << idx << "] (" << x << ", " << y << ") ";
    }
    std::cout << "\n";
    // [0] (0, 0) [1] (1, 0) [2] (0, 1) [3] (1, 1)
}
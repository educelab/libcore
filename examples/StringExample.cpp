#include <iostream>
#include <string>
#include <string_view>

#include "educelab/core/utils/String.hpp"

using namespace educelab;

int main()
{
    // Upper and lower case transformers
    std::string upper{"The quick, brown fox."};
    to_lower(upper);
    std::cout << upper << "\n";  // the quick, brown fox.
    to_upper(upper);
    std::cout << upper << "\n";  // THE QUICK, BROWN FOX.

    // Trim operations
    std::cout << trim_left("  left") << "\n";    // left
    std::cout << trim_right("right  ") << "\n";  // right
    std::cout << trim("  center  ") << "\n";     // center

    // Split space separated
    for (const auto& s : split("  a b c  ")) {
        std::cout << s << " ";
    }
    std::cout << "\n";
    // a b c

    // Split comma separated
    for (const auto& s : split("a,b,c", ',')) {
        std::cout << s << " ";
    }
    std::cout << "\n";
    // a b c

    // Split multiple delimiters
    for (const auto& s : split("a+b-c", '+', '-')) {
        std::cout << s << " ";
    }
    std::cout << "\n";
    // a b c

    // Conversion to numeric types
    std::cout << to_numeric<int>("3.14") << "\n";    // 3
    std::cout << to_numeric<float>("3.14") << "\n";  // 3.14
}
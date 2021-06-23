# EduceLab libcore

A single C++ library for types and utilities shared across the various EduceLab projects.

## Usage

Include the primary header to load all classes:
```{.cpp}
#include <educelab/core.hpp>

using namespace educelab;
```

### N-dimensional vector class
```{.cpp}
#include <educelab/core/types/Vec.hpp>

Vec3f v0{1, 0, 0}, v1{0, 1, 0};
std::cout << v0 + v1 << "\n";         // "[1, 1, 0]"
std::cout << v0.cross(v1) << "\n";    // "[0, 0, 1]"
```

### Iteration utilities
```{.cpp}
#include <educelab/core/utils/Iteration.hpp>

// Numerical ranges
for(const auto& i : range(4, 12, 2)) {
    std::cout << i << " ";
}
std::cout << "\n";                    // "4 6 8 10"

// Indexed ranges
for(const auto& [idx, val] : enumerate("The", "quick", "brown", "fox.")) {
    std::cout << "[" << idx << "] " << val << " ";
}
std::cout << "\n";                    // "[0] The [1] quick [2] brown [3] fox."
```

### String utilities
```{.cpp}
#include <educelab/core/utils/String.hpp>

std::string upper{"THE QUICK BROWN FOX"};
to_lower(upper);
std::cout << upper << "\n";           // "the quick brown fox"
to_upper(upper);
std::cout << upper << "\n";           // "THE QUICK BROWN FOX"
```
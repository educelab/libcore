# EduceLab libcore

A single C++ library for types and utilities shared across the various EduceLab projects.

[[_TOC_]]

## Requirements
- Compiler supporting C++17
- CMake 3.15+

## Build from source

```shell
# Get the source
git clone https://gitlab.com/educelab/libcore.git
cd libcore/

# Build the library
mkdir build
cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
```

## Installation
Follow the build instrcutions above, then run the following command from the root of the source directory:

```shell
# Install the library to the system
cmake --install build/
```

### Header-only classes and utilities
Much of the functionality in this project is header-only and can be copied to your include directory without linkage. 
Note that you may have to adjust the `#include` directives for files which reference other headers in this project. The 
following files can be installed in this way:

- `utils/Iteration.hpp`
- `utils/Math.hpp`
- `utils/String.hpp`
- `types/Vec.hpp`  
    - Requires:
      - `utils/Math.hpp`
- `types/Color.hpp` 
    - Requires: 
      - `types/Vec.hpp`
- `types/Mesh.hpp` 
    - Requires: 
      - `types/Vec.hpp`
      - `types/Color.hpp`


## Usage

Include the primary header to load all classes:
```c++
#include <educelab/core.hpp>

using namespace educelab;
```

### N-dimensional vector class
```c++
#include <educelab/core/types/Vec.hpp>

Vec3f v0{1, 0, 0}, v1{0, 1, 0};
std::cout << v0 + v1 << "\n";         // "[1, 1, 0]"
std::cout << v0.cross(v1) << "\n";    // "[0, 0, 1]"
```

See [examples/VecExample.cpp](examples/VecExample.cpp) for more usage 
examples.

### Iteration utilities
```c++
#include <educelab/core/utils/Iteration.hpp>

// Numerical ranges
for(const auto& i : range(4, 12, 2)) {
    std::cout << i << " ";
}
std::cout << "\n";    // "4 6 8 10"

// Indexed ranges
for(const auto& [idx, val] : enumerate("The", "quick", "brown", "fox.")) {
    std::cout << "[" << idx << "] " << val << " ";
}
std::cout << "\n";    // "[0] The [1] quick [2] brown [3] fox."
```

See [examples/IterationExample.cpp](examples/IterationExample.cpp) for more 
usage examples.

### String utilities
```c++
#include <educelab/core/utils/String.hpp>

// Upper and lower case transformers
std::string upper{"The quick brown fox"};
to_lower(upper);
std::cout << upper << "\n";    // "the quick brown fox"
to_upper(upper);
std::cout << upper << "\n";    // "THE QUICK BROWN FOX"

// Trim operations
std::cout << trim_left("  left") << "\n";      // "left"
std::cout << trim_right("right  ") << "\n";    // "right"
std::cout << trim("  center  ") << "\n";       // "center"

// Conversion to numeric types
std::cout << to_numeric<int>("3.14") << "\n";   // 3
std::cout << to_numeric<float>("3.14") << "\n"; // 3.14
```

See [examples/StringExample.cpp](examples/StringExample.cpp) for more usage
examples.
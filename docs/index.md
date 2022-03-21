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

### Dense 2D matrix class

```{.cpp}
#include <educelab/core/types/Mat.hpp>
#include <educelab/core/types/Vec.hpp>

// Input point
Vec<float, 4> p{0, 0, 0, 1};
std::cout << p << "\n";          // [0, 0, 0, 1]

// Construct a translation matrix
auto M = Mat<4,4>::Eye();
M(0, 3) = 1.f;
M(1, 3) = 2.f;
M(2, 3) = 3.f;
std::cout << "\n" << M << "\n";  // [[1, 0, 0, 1]
                                 //  [0, 1, 0, 2]
                                 //  [0, 0, 1, 3]
                                 //  [0, 0, 0, 1]]

// Apply transform               
p = translate * p;
std::cout << p << "\n";          // [1, 2, 3, 1]
```

### Image class

```{.cpp}
#include <educelab/core/types/Image.hpp>
#include <educelab/core/types/Vec.hpp>

// Construct an image
Image image(600, 800, 3, Depth::F32);

// Fill image with a color gradient
for (const auto [y, x] : range2D(image.height(), image.width())) {
    auto r = float(x) / float(image.width() - 1);
    auto g = float(y) / float(image.height() - 1);
    auto b = 0.25F;
    image.at<Vec3f>(y, x) = {r, g, b};
}
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

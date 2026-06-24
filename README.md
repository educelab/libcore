# EduceLab libcore

A single C++ library for types and utilities shared across the various EduceLab projects.

[![](https://www.gitlab.com/educelab/libcore/badges/develop/pipeline.svg?ignore_skipped=true)](https://gitlab.com/educelab/libcore/-/commits/develop)
[![](https://img.shields.io/static/v1?label=&message=docs&color=orange)](https://educelab.gitlab.io/libcore/docs/)

## Requirements
- Compiler supporting C++17
- CMake 3.15+

## Build from source

```shell
# Get the source
git clone https://github.com/educelab/libcore.git
cd libcore/

# Build the library
cmake -S . -B build/ -DCMAKE_BUILD_TYPE=Release
cmake --build build/
```

## Installation
Follow the build instructions above, then run the following command from the root of the source directory:

```shell
# Install the library to the system
cmake --install build/
```

### Header-only classes and utilities
Much of the functionality in this project is header-only and can be copied to your include directory without linkage.
Note that you may have to adjust the `#include` directives for files which reference other headers in this project. The
following files can be installed in this way:

- `utils/Caching.hpp`
- `utils/Iteration.hpp`
- `utils/Math.hpp`
- `utils/String.hpp`
- `utils/Filesystem.hpp`
    - Requires:
      - `utils/String.hpp`
- `utils/Flags.hpp`
- `utils/LinearAlgebra.hpp`
    - Requires:
      - `utils/Math.hpp`
      - `MatrixType` and `VectorType` which implement the `Mat` and `Vec`
        interfaces.
- `types/Signals.hpp`
- `types/Vec.hpp`  
    - Requires:
      - `utils/Math.hpp`
- `types/Mat.hpp`
    - Requires:
      - `types/Vec.hpp`
- `types/Color.hpp`
    - Requires:
      - `types/Vec.hpp`
- `types/Mesh.hpp`
    - Requires:
      - `types/Vec.hpp`
      - `types/Color.hpp`
- `types/UVMap.hpp`
    - Requires:
      - `types/Vec.hpp`
- `utils/MeshUtils.hpp`
    - Requires:
      - `types/Mesh.hpp`
      - `types/UVMap.hpp`
- `io/MeshIO_OBJ.hpp`
    - Requires:
      - `types/Mesh.hpp`
      - `types/UVMap.hpp`
      - `utils/String.hpp`
- `io/MeshIO_PLY.hpp`
    - Requires:
      - `types/Mesh.hpp`
      - `types/UVMap.hpp`
      - `utils/String.hpp`
- `io/MeshIO.hpp`
    - Convenience facade; includes `MeshIO_OBJ.hpp` and `MeshIO_PLY.hpp`


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

### Dense 2D matrix class

```c++
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

See [examples/MatExample.cpp](examples/MatExample.cpp) for more usage
examples.

### Mesh class

```c++
#include "educelab/core/types/Mesh.hpp"

// Default mesh: 3D float positions, no extra traits
Mesh<float, 3> mesh;

auto v0 = mesh.insert_vertex(0.F, 0.F, 0.F);
auto v1 = mesh.insert_vertex(1.F, 0.F, 0.F);
auto v2 = mesh.insert_vertex(1.F, 1.F, 0.F);
auto v3 = mesh.insert_vertex(0.F, 1.F, 0.F);

// N-gon faces are supported
auto f0 = mesh.insert_face(v0, v1, v2, v3);

std::cout << mesh.vertex(v0) << "\n";       // [0, 0, 0]
std::cout << mesh.face_normal(f0) << "\n";  // [0, 0, 1]

// Adjacency index (lazy, cached)
for (auto fi : mesh.vertex_faces(v1)) {
    std::cout << "face " << fi << "\n";     // face 0
}
```

Opt-in per-vertex traits compose via multiple inheritance:

```c++
#include "educelab/core/types/Mesh.hpp"

struct MyTraits : traits::WithNormal<float, 3>, traits::WithColor {};
using RichMesh = Mesh<float, 3, MyTraits>;

RichMesh rich;
auto v = rich.insert_vertex(0.F, 0.F, 0.F);
rich.vertex(v).normal = Vec3f{0.F, 0.F, 1.F};
rich.vertex(v).color  = Color::U8C3{255, 0, 0};
```

See [examples/MeshExample.cpp](examples/MeshExample.cpp) for more usage examples.

### UVMap class

`UVMap` stores per-wedge UV coordinates as a flat coordinate pool plus a
per-face, per-corner index into that pool — mirroring the OBJ/PLY `vt` index
structure. Multiple corners may reference the same pool entry; the same vertex
can map to different UVs in adjacent faces (seam support).

```c++
#include "educelab/core/types/UVMap.hpp"

UVMap<float, 2> uvmap;

// Insert coordinates into the pool
auto uv0 = uvmap.insert(0.0F, 0.0F);
auto uv1 = uvmap.insert(1.0F, 0.0F);
auto uv2 = uvmap.insert(1.0F, 1.0F);

// Assign pool entries to wedges (face, corner, pool index)
uvmap.map(0, 0, uv0);
uvmap.map(0, 1, uv1);
uvmap.map(0, 2, uv2);

// Look up the coordinate for a wedge
const auto& c = uvmap.get_coordinate(0, 2);
std::cout << "[" << c[0] << ", " << c[1] << "]\n";  // [1, 1]
```

Attach a chart index to each coordinate for multi-texture atlases:

```c++
using ChartedUVMap = UVMap<float, 2, traits::WithChart>;
ChartedUVMap charted;

ChartedUVMap::Coordinate c(0.25F, 0.25F);
c.chart = 1;
auto idx = charted.insert(c);
charted.map(0, 0, idx);

std::cout << charted.get_coordinate(0, 0).chart << "\n";  // 1
```

See [examples/MeshExample.cpp](examples/MeshExample.cpp) for more usage examples.

### Mesh IO

Read and write OBJ and PLY files using the convenience facade:

```c++
#include "educelab/core/io/MeshIO.hpp"

Mesh<float, 3> mesh;

// Read — format detected from file extension (.obj or .ply)
read_mesh("model.obj", mesh);

// Write
write_mesh("output.ply", mesh);
```

Read and write with UV maps and texture paths:

```c++
#include "educelab/core/io/MeshIO.hpp"

Mesh<float, 3> mesh;
UVMap<float, 2> uvmap;
std::vector<std::filesystem::path> texture_paths;

// Read UV coords and texture path references
read_mesh("model.obj", mesh, uvmap, texture_paths);

// Write with a single texture path (OBJ emits .mtl; PLY emits comment TextureFile)
write_mesh("output.obj", mesh, uvmap, texture_paths[0]);
```

Vertex traits are detected at compile time — normals and colors are read/written
automatically when the vertex type includes them:

```c++
struct MyTraits : traits::WithNormal<float, 3>, traits::WithColor {};
using RichMesh = Mesh<float, 3, MyTraits>;

RichMesh mesh;
read_mesh("model.ply", mesh);  // normals and colors populated if present in file
write_mesh("output.ply", mesh); // normals and colors included in output
```

### Image class

```c++
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

See [examples/ImageExample.cpp](examples/ImageExample.cpp) for more usage
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

// Split on whitespace (any run treated as one delimiter)
auto tokens = split("  v  1.0\t2.0  3.0  ");  // {"v", "1.0", "2.0", "3.0"}

// Split with a predicate (single-pass, no allocation)
auto parts = split("1/2/3", [](char c) { return c == '/'; }); // {"1", "2", "3"}

// Conversion from string to numeric types
std::cout << to_numeric<int>("3.14") << "\n";   // 3
std::cout << to_numeric<float>("3.14") << "\n"; // 3.14

// Conversion from numeric to string (locale-independent)
std::cout << to_string(3.14f) << "\n";          // "3.14"

// Buffer-reusing variant for hot paths (no heap allocation)
std::array<char, 128> buf;
file << to_string_view(buf, x) << ' ' << to_string_view(buf, y) << '\n';
```

See [examples/StringExample.cpp](examples/StringExample.cpp) for more usage
examples.

#### A note on `to_numeric` and `to_string[_view]` compilation

`to_numeric` uses `std::from_chars` for string-to-number conversion.
`to_string` and `to_string_view` use `std::to_chars` for number-to-string
conversion. Both were introduced in C++17 but floating-point support arrived
later and is gated on the runtime library version (e.g. macOS 13.3+).

CMake probes for both at configure time via `CheckCharconvFP.cmake` and reports
the results as compile definitions (only emitted when the corresponding
function+type combination is absent):

- **`from_chars` fallbacks** — `to_numeric` falls back to `std::stof` /
  `std::stod` / `std::stold` for any type whose definition is set:
  - `EDUCE_CORE_NEED_FROM_CHARS_FLOAT`
  - `EDUCE_CORE_NEED_FROM_CHARS_DOUBLE`
  - `EDUCE_CORE_NEED_FROM_CHARS_LONG_DOUBLE`
- **`to_chars` fallbacks** — `to_string` / `to_string_view` fall back to
  `std::snprintf` for any type whose definition is set:
  - `EDUCE_CORE_NEED_TO_CHARS_FLOAT`
  - `EDUCE_CORE_NEED_TO_CHARS_DOUBLE`
  - `EDUCE_CORE_NEED_TO_CHARS_LONG_DOUBLE`

These definitions are attached to the `educelab::core` target as `PUBLIC`
compile definitions, so they propagate automatically to any target that links
against it:

```cmake
# Import libcore
FetchContent_Declare(
    libcore
    GIT_REPOSITORY https://github.com/educelab/libcore.git
    GIT_TAG v0.3.0
    EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(libcore)

# Definitions are propagated automatically — no extra step needed.
add_executable(foo foo.cpp)
target_link_libraries(foo PRIVATE educelab::core)
```

Alternatively, header-only functionality can be compiled into your binary 
without linking against the library by adding the libcore's include directory 
to your target's set of include directories:
```cmake
target_include_directories(foo
    $<BUILD_INTERFACE:${libcore_SOURCE_DIR}/include>
)
```

> [!NOTE]
> This means that `#include <educelab/core/{Header}.hpp` must only appear in 
> `.cpp` files.

If using the headers without linking against the CMake target, you must also 
manually set any required definitions:

```cmake
target_compile_definitions(foo PRIVATE EDUCE_CORE_NEED_FROM_CHARS_LONG_DOUBLE)
```

### Data caching

```c++
#include <educelab/core/utils/Iteration.hpp>
#include <educelab/core/utils/Caching.hpp>

// Create cache
using Key = ObjectCache<>::key_type;
ObjectCache cache;

// Store 5 ints and floats
std::vector<Key> keys;
for (auto val : range(5)) {
    keys.emplace_back(cache.insert(val));
    keys.emplace_back(cache.insert(0.5f + val));
}

// Print cached values
for (const auto& k : keys) {
    // Check that the key is still in the cache
    if (not cache.contains(k)) {
        continue;
    }
    
    // Get the value and cast to the correct type
    auto val = cache.get(k);
    if (val.type() == typeid(int)) {
        std::cout << std::any_cast<int>(val) << " ";
    } 
    
    else if (val.type() == typeid(float)) {
        std::cout << std::any_cast<float>(val) << " ";
    } 
} 
std::cout << "\n";  // 0 0.5 1 1.5 2 2.5 3 3.5 4 4.5
```

See [examples/CachingExample.cpp](examples/CachingExample.cpp) for more usage
examples.

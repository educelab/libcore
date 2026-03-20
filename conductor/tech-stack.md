# Tech Stack

## Language

- **C++17** — minimum standard required (`cxx_std_17`)

## Build System

- **CMake 3.15+** — primary build system
- **FetchContent** — dependency management for third-party libs

## Testing

- **GoogleTest** — unit test framework, fetched via FetchContent
- Tests are gated behind `EDUCE_CORE_BUILD_TESTS` CMake option

## Documentation

- **Doxygen** — API documentation generation (optional)
- Enabled via `EDUCE_CORE_BUILD_DOCS` CMake option when Doxygen is found

## Key Library Components

| Component | Header |
|-----------|--------|
| Vec | `types/Vec.hpp` |
| Mat | `types/Mat.hpp` |
| Image | `types/Image.hpp` |
| Mesh | `types/Mesh.hpp` |
| Color | `types/Color.hpp` |
| Signals | `types/Signals.hpp` |
| Uuid | `types/Uuid.hpp` |
| Math | `utils/Math.hpp` |
| LinearAlgebra | `utils/LinearAlgebra.hpp` |
| String | `utils/String.hpp` |
| Filesystem | `utils/Filesystem.hpp` |
| Caching | `utils/Caching.hpp` |
| Iteration | `utils/Iteration.hpp` |
| ImageIO | `io/ImageIO.hpp` |

## Distribution

- **CMake FetchContent / find_package** — primary integration method for consumers
- **System package managers** (apt, brew) — secondary distribution channel

## Tooling

- **clang-format** — C++ code formatting
- **clang-tidy** — C++ static analysis
- **GitHub Actions** — CI/CD

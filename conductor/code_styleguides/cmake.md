# CMake Style Guide

## General Principles

- Prefer modern CMake (target-based) patterns — avoid directory-level commands
- Keep `CMakeLists.txt` files readable and well-commented
- Use CMake 3.15+ features freely; do not support older versions

---

## Naming Conventions

| Entity | Convention | Example |
|--------|-----------|---------|
| Targets | `snake_case` | `educelab_core`, `core` |
| Target aliases | `Namespace::PascalCase` | `educelab::core` |
| Variables | `SCREAMING_SNAKE_CASE` | `EDUCE_CORE_BUILD_TESTS` |
| Options/Cache vars | `SCREAMING_SNAKE_CASE` | `EDUCE_CORE_BUILD_DOCS` |
| Functions/Macros | `snake_case` | `educe_add_test()` |
| CMake modules | `PascalCase.cmake` | `InstallProject.cmake` |

---

## Project Structure

```cmake
cmake_minimum_required(VERSION 3.15 FATAL_ERROR)
project(MyProject VERSION 1.0.0)

# Options at the top
option(MY_BUILD_TESTS "Build tests" OFF)

# Dependencies
include(FetchContent)
FetchContent_Declare(...)
FetchContent_MakeAvailable(...)

# Targets
add_library(mylib ...)
add_library(mynamespace::mylib ALIAS mylib)

# Target properties
target_include_directories(mylib PUBLIC ...)
target_compile_features(mylib PUBLIC cxx_std_17)
target_link_libraries(mylib PUBLIC ...)

# Subdirectories at the bottom
if(MY_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
```

---

## Targets

- Always create an alias target with a namespace: `add_library(ns::target ALIAS target)`
- Use `target_*` commands — never `include_directories()`, `link_libraries()`, etc.
- Prefer `PUBLIC` / `PRIVATE` / `INTERFACE` scoping deliberately:
  - `PUBLIC` — needed by both the target and its consumers
  - `PRIVATE` — needed only by the target's own build
  - `INTERFACE` — needed only by consumers (e.g. header-only libs)

---

## Options and Cache Variables

- Prefix all project options with the project name: `EDUCE_CORE_BUILD_TESTS`
- Provide a description string: `option(EDUCE_CORE_BUILD_TESTS "Build unit tests" OFF)`
- Use `CMAKE_DEPENDENT_OPTION` for options that depend on other options
- Default optional features to `OFF` unless they are always needed

---

## FetchContent

```cmake
include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.14.0  # Always pin to a tag or commit SHA
)

FetchContent_MakeAvailable(googletest)
```

- Always pin dependencies to a specific tag or commit SHA — never use `main` or `HEAD`
- Declare all dependencies before calling `FetchContent_MakeAvailable`

---

## Install Rules

- Use `GNUInstallDirs` for standard install paths
- Export targets so consumers can use `find_package`
- Install headers with `PUBLIC_HEADER DESTINATION`

```cmake
include(GNUInstallDirs)

install(
    TARGETS mylib
    EXPORT MyLibTargets
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/mylib"
)
```

---

## Testing

- Gate tests behind an option: `option(MY_BUILD_TESTS "Build tests" OFF)`
- Call `enable_testing()` before `add_subdirectory(tests)`
- Use `add_test()` or `gtest_discover_tests()` for test registration
- Name test targets descriptively: `myproject_TestVec`, `myproject_TestString`

---

## Formatting

- Indent with 4 spaces (no tabs)
- One argument per line for multi-argument commands
- Align closing parenthesis with the opening command:

```cmake
target_include_directories(mylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
```

- Use blank lines to separate logical sections
- Comment non-obvious decisions

---

## Anti-patterns to Avoid

- `file(GLOB ...)` for source lists — list files explicitly
- `include_directories()` — use `target_include_directories()` instead
- `link_libraries()` — use `target_link_libraries()` instead
- `add_definitions()` — use `target_compile_definitions()` instead
- Setting `CMAKE_CXX_STANDARD` globally — use `target_compile_features()` instead
- Hardcoded paths — use CMake variables and generator expressions

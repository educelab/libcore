# C++ Style Guide

## General Principles

- Prefer clarity over cleverness
- Follow existing patterns in the codebase
- Minimize unnecessary complexity
- C++17 standard; avoid features not yet widely supported

---

## Naming Conventions

| Entity | Convention | Example |
|--------|-----------|---------|
| Types / Classes | `PascalCase` | `Vec3d`, `ImageIO` |
| Functions / Methods | `camelCase` | `splitString()`, `toNumeric()` |
| Variables | `camelCase` | `imageWidth`, `numChannels` |
| Constants | `SCREAMING_SNAKE_CASE` | `MAX_CHANNELS` |
| Template parameters | `PascalCase` | `typename Scalar` |
| Namespaces | `lowercase` | `educelab::core` |
| Private members | `camelCase_` (trailing underscore) | `data_`, `width_` |
| File names | `PascalCase.hpp` / `PascalCase.cpp` | `Vec.hpp`, `Image.cpp` |

---

## File Structure

### Header files (`.hpp`)

```cpp
#pragma once

#include <system_headers>
#include "project_headers"

namespace educelab {

class MyType {
public:
    // Public API first
    MyType() = default;

    int value() const { return value_; }
    void setValue(int v) { value_ = v; }

private:
    int value_{0};
};

}  // namespace educelab
```

### Source files (`.cpp`)

```cpp
#include "educelab/core/types/MyType.hpp"

#include <system_headers>

namespace educelab {

// Implementation

}  // namespace educelab
```

---

## clang-format Configuration

Place a `.clang-format` file at the project root:

```yaml
BasedOnStyle: Google
IndentWidth: 4
ColumnLimit: 100
AccessModifierOffset: -4
ConstructorInitializerIndentWidth: 4
ContinuationIndentWidth: 4
AlignConsecutiveAssignments: false
AlignConsecutiveDeclarations: false
BraceWrapping:
  AfterClass: false
  AfterFunction: false
  AfterNamespace: false
BreakBeforeBraces: Attach
DerivePointerAlignment: false
PointerAlignment: Left
SpaceBeforeParens: ControlStatements
SortIncludes: true
IncludeBlocks: Regroup
IncludeCategories:
  - Regex: '^<.*>'
    Priority: 1
  - Regex: '^".*"'
    Priority: 2
```

---

## clang-tidy Configuration

Place a `.clang-tidy` file at the project root:

```yaml
Checks: >
  clang-diagnostic-*,
  clang-analyzer-*,
  cppcoreguidelines-*,
  modernize-*,
  performance-*,
  readability-*,
  -modernize-use-trailing-return-type,
  -cppcoreguidelines-avoid-magic-numbers,
  -readability-magic-numbers,
  -cppcoreguidelines-pro-bounds-pointer-arithmetic

WarningsAsErrors: ''

HeaderFilterRegex: 'include/educelab/.*'

CheckOptions:
  - key: readability-identifier-naming.ClassCase
    value: CamelCase
  - key: readability-identifier-naming.FunctionCase
    value: camelBack
  - key: readability-identifier-naming.VariableCase
    value: camelBack
  - key: readability-identifier-naming.MemberSuffix
    value: '_'
  - key: readability-identifier-naming.PrivateMemberSuffix
    value: '_'
  - key: readability-identifier-naming.NamespaceCase
    value: lower_case
  - key: readability-identifier-naming.TemplateParameterCase
    value: CamelCase
```

---

## Headers and Includes

- Use `#pragma once` — no include guards
- Group includes in order: system → third-party → project, separated by blank lines
- Use angle brackets for system/third-party headers, quotes for project headers
- Forward-declare where possible to minimize header dependencies

---

## Classes and Structs

- Use `struct` for plain data aggregates with no invariants
- Use `class` when encapsulation or invariants are required
- Order members: `public` → `protected` → `private`
- Initialize all member variables at declaration or in constructor initializer list
- Prefer `= default` / `= delete` over empty or deleted implementations

---

## Functions

- Keep functions short and single-purpose
- Prefer returning values over output parameters
- Use `const` wherever applicable (methods, references, pointers)
- Mark single-argument constructors `explicit` unless implicit conversion is intentional

---

## Templates

- Document template parameters with a brief comment
- Prefer `typename` over `class` for template type parameters
- Use `static_assert` to enforce constraints on template parameters

---

## Error Handling

- Prefer return values or `std::optional` over exceptions for expected failure modes
- Use exceptions only for truly exceptional, unrecoverable conditions
- Never swallow exceptions silently

---

## Memory Management

- Prefer stack allocation and value semantics
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) — never raw `new`/`delete`
- Avoid unnecessary heap allocation in hot paths

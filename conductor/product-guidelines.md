# Product Guidelines

## Voice and Tone

**Concise and direct.** Documentation, API comments, and error messages should say exactly what is needed — nothing more. Prefer short sentences and precise technical language. Avoid filler phrases.

## Design Principles

### Developer Experience Focused
APIs should be intuitive and unsurprising. Prefer conventional C++ idioms. Minimize boilerplate required by consumers. Header-friendly design is preferred where practical.

### Minimize Dependencies
External dependencies should be rare and justified. Prefer implementing functionality in-house when the dependency would be heavy or have transitive costs. When a dependency is required, prefer fetching via CMake FetchContent with pinned versions.

## API Standards

- Public headers live under `include/educelab/core/`
- Types go in `include/educelab/core/types/`
- Utilities go in `include/educelab/core/utils/`
- All public API must be covered by unit tests before merging

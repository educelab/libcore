# Specification: UVMap

**Track ID:** uv-map_20260323
**Type:** Feature
**Created:** 2026-03-23
**Status:** Draft

## Summary

Introduce a `UVMap<T, Dims, Traits>` class template providing per-wedge
(per face-corner) UV/UVW coordinate storage as a companion to `Mesh`.
Generalized from the usage patterns in registration-toolkit, but redesigned as
a header-only, OpenCV-free, N-gon-capable class template with a two-level API
mirroring the OBJ/PLY index structure and an opt-in traits system for
per-coordinate metadata.

## Context

EduceLab Core provides common types for EduceLab C++ projects. UV coordinates
must be stored per-wedge — not per-vertex — to correctly represent UV seams,
where the same vertex position carries different UV coordinates in adjacent
faces. `UVMap` is a standalone companion to `Mesh` (not embedded in it) so that
meshes that do not require UV data carry no overhead.

The `Dims` template parameter generalizes the class beyond 2D texture space:
`UVMap<>` (defaulting to `float, 2`) is a standard UV map; `UVMap<float, 3>`
is a UVW map for volumetric textures. `T` allows callers to choose float vs.
double precision. `Traits` allows opt-in per-coordinate metadata via the same
mixin pattern used by `Mesh::Vertex`.

## User Story

As an EduceLab developer, I want a per-wedge UV/UVW coordinate store that
accompanies a `Mesh` so that I can represent UV-mapped textures and atlases
with correct seam handling when reading and writing mesh files, regardless of
coordinate dimensionality, and optionally attach per-coordinate metadata.

## Acceptance Criteria

- [ ] `UVMap<T, Dims, Traits>` stores a pool of `Coordinate` objects (see
      Technical Notes); coordinates are inserted via
      `insert(Coordinate) -> std::size_t`, a `Vec`-accepting overload
      `insert(Vec<T,Dims>) -> std::size_t`, and a variadic overload
      `insert(T u, T v, ...) -> std::size_t`; retrieved by index via
      `at(std::size_t) -> const Coordinate&`, which throws `std::out_of_range`
      for an out-of-bounds index
- [ ] Per-wedge mapping is set via `map(face, corner, uvIdx)` (auto-grows
      storage) and queried via `get(face, corner) -> std::size_t` (returns the
      pool index); both `face` and `corner` are `std::size_t`
- [ ] `get_coordinate(face, corner) -> const Coordinate&` is a convenience
      wrapper equivalent to `at(get(face, corner))`
- [ ] `get` and `get_coordinate` throw `std::out_of_range` for any unmapped
      wedge; `has(face, corner) -> bool` is the sanctioned pre-check
- [ ] `size() -> std::size_t` returns the UV coordinate pool size; `empty() ->
      bool` returns whether the pool is empty
- [ ] `reserve_uvs(std::size_t)` pre-allocates the UV pool; `reserve_faces(
      std::size_t)` pre-allocates the outer face entries in `face_uvs_`
- [ ] `clear()` resets both the UV pool and the per-wedge mapping to empty
- [ ] `UVMap<T, Dims, Traits>` is default-constructible and copyable/movable
- [ ] Template parameters default to `T = float`, `Dims = 2`,
      `Traits = DefaultUVTraits`; `UVMap<>` is a standard 2D float UV map
- [ ] A `WithChart` mixin is provided: `struct WithChart { std::size_t
      chart{0}; }`; composable as `UVMap<float, 2, WithChart>`
- [ ] All additions are C++17 compatible and header-only
- [ ] Tests achieve ≥80% coverage of all public methods

## Dependencies

- `Vec.hpp` (no change required)
- No dependency on `mesh-redesign_20260320` — `UVMap` is independent of `Mesh`

## Out of Scope

- UV unwrapping / parameterization (OpenABF's responsibility)
- Multiple UV channels per mesh (future feature)
- UV coordinate validation (e.g. range checks on [0,1])
- Embedding `UVMap` inside `Mesh`
- Origin-flip conversion (TopLeft/BottomLeft) — callers handle coordinate
  system transformation at IO boundaries
- Per-map metadata such as texture dimensions (caller's responsibility)
- Convenience type aliases

## Technical Notes

### `Coordinate` Inner Struct

`UVMap<T, Dims, Traits>` defines an inner struct `Coordinate` that inherits
from both `Vec<T,Dims>` and `Traits`, mirroring the design of `Mesh::Vertex`:

```cpp
struct Coordinate : public Vec<T, Dims>, public Traits {
    Coordinate() = default;

    template <typename... Args>
    explicit Coordinate(Args... args) : Vec<T, Dims>{args...} {}

    using Vec<T, Dims>::operator=;

    // operator+=, -=, *=, /= returning Coordinate& (not Vec&)
    // operator+, -, *, / as friend functions returning Coordinate
    // (same set as Mesh::Vertex — prevents slicing of Traits fields)
};
```

The arithmetic overloads are necessary to prevent slicing: `Vec`'s operators
return `Vec` / `Vec&`, which would discard any `Traits` fields on copy or
assignment. `Coordinate`'s overloads shadow them and return `Coordinate` /
`Coordinate&`.

### Traits

```cpp
// Default: no extra per-coordinate data
struct DefaultUVTraits {};

// Opt-in chart index
struct WithChart { std::size_t chart{0}; };
```

Callers compose traits via multiple inheritance if needed:
```cpp
struct MyTraits : WithChart { /* ... */ };
using MyUVMap = UVMap<float, 2, MyTraits>;
```

### API Summary

```cpp
template <typename T = float, std::size_t Dims = 2,
          typename Traits = DefaultUVTraits,
          std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
class UVMap {
public:
    struct Coordinate : public Vec<T,Dims>, public Traits { /* ... */ };

    // UV pool
    auto insert(const Coordinate&) -> std::size_t;
    auto insert(const Vec<T,Dims>&) -> std::size_t;
    template <typename... Args>
    auto insert(Args...) -> std::size_t;              // e.g. insert(0.5f, 0.25f)
    auto at(std::size_t) -> Coordinate&;              // throws out_of_range
    auto at(std::size_t) const -> const Coordinate&;  // throws out_of_range

    // Per-wedge mapping
    void map(std::size_t face, std::size_t corner, std::size_t uvIdx);
    auto get(std::size_t face, std::size_t corner) -> std::size_t;
    auto get_coordinate(std::size_t face, std::size_t corner) -> Coordinate&;
    auto get_coordinate(std::size_t face, std::size_t corner) const
        -> const Coordinate&;
    auto has(std::size_t face, std::size_t corner) -> bool;

    // Container
    auto size() -> std::size_t;
    auto empty() -> bool;
    void reserve_uvs(std::size_t);
    void reserve_faces(std::size_t);
    void clear();
};
```

### Internal Storage

- `std::vector<Coordinate> uvs_` — the UV coordinate pool
- `std::vector<std::vector<std::optional<std::size_t>>> face_uvs_` — per-face,
  per-corner UV pool index; `face_uvs_[f][c]` is `std::nullopt` if unmapped

### Growth Behavior of `map`

`map(face, corner, idx)` performs:
1. `face_uvs_.resize(face + 1, {})` — grows outer vector if needed
2. `face_uvs_[face].resize(corner + 1, std::nullopt)` — grows inner vector if needed
3. `face_uvs_[face][corner] = idx`

Corners may be mapped in any order; gaps remain `std::nullopt` until filled.
`face_uvs_[face].size()` reflects the highest corner index mapped so far, not
the face's canonical corner count — `has()` is the correct query.

### Two-Level Design

The pool + per-wedge index structure mirrors the OBJ/PLY file formats' separate
`vt` index space: multiple wedges can reference the same pool entry (no
duplication), and the same vertex can map to different UV coordinates in
different faces (seam handling). The mesh-io track uses `insert`/`map` directly
when reading files (indices are explicit). Programmatic construction uses vertex
indices as deduplication keys:

```cpp
std::vector<std::size_t> vert_to_uv(mesh.num_vertices());
for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi)
    vert_to_uv[vi] = uvmap.insert(compute_uv(vi));
for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
    const auto& f = mesh.face(fi);
    for (std::size_t ci = 0; ci < f.size(); ++ci)
        uvmap.map(fi, ci, vert_to_uv[f[ci]]);
}
```

### Design Contrast With rt::UVMap

rt::UVMap stores UV coordinates per-vertex, breaking down at UV seams.
`UVMap<T,Dims,Traits>` uses per-wedge indexing, which maps directly to OBJ/PLY.
rt::UVMap is hardcoded to triangles, requires OpenCV, and is not templated.
`UVMap<T,Dims,Traits>` removes all three constraints.

---

_Generated by Conductor. Review and edit as needed._

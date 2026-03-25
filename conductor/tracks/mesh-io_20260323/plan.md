# Implementation Plan: Mesh IO

**Track ID:** mesh-io_20260323
**Spec:** [spec.md](./spec.md)
**Created:** 2026-03-23
**Updated:** 2026-03-25
**Status:** [~] In Progress

## Overview

Four phases in dependency order: first establish the detection idiom
infrastructure shared by all IO headers (`has_normal`, `has_color`,
`has_chart`), then implement OBJ IO (including MTL texture path handling and
multi-chart static_assert), then PLY IO (including `expand_at_seams` and
`comment TextureFile` texture path handling), then the convenience facade
(`read_mesh` / `write_mesh`) that dispatches by file extension.

**Preparatory work (complete):** `2f67926` — `String.hpp` extensions: predicate
`split()` overload (O(n), single-character delimiters), no-arg `split()` for
whitespace splitting, `to_string_view(buf, val)` for zero-allocation numeric
serialisation, `to_string(val)` convenience wrapper. `CheckCharconvFP.cmake`
unified and split into separate `from_chars` / `to_chars` probes.

## Checkpoints

| Phase   | Checkpoint SHA | Date | Status  |
| ------- | -------------- | ---- | ------- |
| Phase 1 | e818d3b        | 2026-03-25 | complete |
| Phase 2 |                |      | pending |
| Phase 3 |                |      | pending |
| Phase 4 |                |      | pending |

---

## Phase 1: Trait Detection Infrastructure

Introduce the compile-time detection helpers that OBJ and PLY IO will share.
All three traits (`has_normal`, `has_color`, `has_chart`) live in
`detail/MeshTraits.hpp` and must be documented with opt-in examples.

### Tasks

- [x] **Task 1.1**: Write tests in `tests/src/TestMeshIO.cpp` that verify
      `has_normal<V>`, `has_color<V>`, and `has_chart<UVMapT>` resolve
      correctly for types with and without those traits (compile-time
      assertions via `static_assert`) `e818d3b`
- [x] **Task 1.2**: Create `include/educelab/core/types/detail/MeshTraits.hpp`
      with `has_normal<V>`, `has_color<V>`, and `has_chart<UVMapT>` detection
      traits using `std::void_t`; document each trait with a Doxygen comment
      that shows the opt-in pattern and explains that IO functions use them
      via `if constexpr` `e818d3b`

### Verification

- [x] `ctest` passes with no regressions
- [x] Detection trait `static_assert` tests pass
- [x] Build succeeds in Debug and Release

---

## Phase 2: OBJ IO

Implement `include/educelab/core/io/MeshIO_OBJ.hpp` with `read_obj` and
`write_obj` across all three tiers. Vertex colors use the inline `v x y z r
g b` convention. Multi-chart `write_obj` enforces `has_chart` via
`static_assert`.

**Parsing utilities:** use `split()` with no arguments (from `String.hpp`)
to tokenise whitespace-delimited lines; pass a predicate to `split()` for
single-character delimiters (e.g. `split(token, [](char c){ return c=='/'; })`
for OBJ face vertex references `v/vt/vn`); use `to_numeric<T>` for
string-to-number conversion. **Write utilities:** declare one
`std::array<char, 128> buf` at the top of each write function and pass it to
`to_string_view(buf, val)` for every coordinate — no heap allocation per
conversion.

### Tasks

- [ ] **Task 2.1**: Write round-trip tests for OBJ in `TestMeshIO.cpp` —
      cover: positions only; positions + normals (`WithNormal` mesh);
      positions + colors (`WithColor` mesh, verify inline `v x y z r g b`
      format); positions + UVs (no texture); positions + UVs + single texture
      path (verify `.mtl` written and path round-trips); positions + UVs +
      multi-chart texture paths via `WithChart` (verify per-chart `usemtl`
      grouping and `texture_paths` vector recovery); positions + UVs + multi-
      chart where `read_obj` populates chart indices only when `has_chart`;
      N-gon faces; mesh type without normals reading a file that has normals
      (normals silently ignored); missing file error; `.mtl` present but no
      `map_Kd` (empty `texture_paths`)
- [ ] **Task 2.2**: Implement `write_obj(path, mesh)` — writes `v` lines with
      optional inline RGB (`if constexpr has_color<Vertex>`); writes `vn`
      lines (`if constexpr has_normal<Vertex>`); writes `f` lines
- [ ] **Task 2.3**: Implement `write_obj(path, mesh, uvmap)` — adds `vt`
      lines and per-wedge UV indices in `f` lines; no `.mtl` emitted
- [ ] **Task 2.4**: Implement `write_obj(path, mesh, uvmap, texture_path)` —
      single `std::filesystem::path`; emits a `.mtl` with one material
      (`newmtl material0`, `map_Kd <path>`); all faces prefixed with
      `usemtl material0`
- [ ] **Task 2.5**: Implement `write_obj(path, mesh, uvmap, texture_paths)` —
      `std::vector<std::filesystem::path>` overload;
      `static_assert(has_chart<UVMapT>::value, "write_obj with multiple
      texture paths requires UVMap with traits::WithChart")`; emits one
      `newmtl materialN` / `map_Kd` entry per path; groups faces by the chart
      index of corner 0 with `usemtl materialN` directives
- [ ] **Task 2.6**: Implement `read_obj(path, mesh)` — parses `v` lines with
      3 or 6+ components (position only vs position + RGB); parses `vn`, `f`;
      populates normals via `if constexpr has_normal<Vertex>`; populates
      colors via `if constexpr has_color<Vertex>`; silently skips
      unrecognized directives
- [ ] **Task 2.7**: Implement `read_obj(path, mesh, uvmap)` — also parses
      `vt` and per-wedge UV indices from `f` lines; populates `uvmap`;
      populates chart indices on `uvmap` coordinates via
      `if constexpr has_chart<UVMapT>` using material group ordering
- [ ] **Task 2.8**: Implement `read_obj(path, mesh, uvmap, texture_paths)` —
      parses `.mtl` referenced by `mtllib` directive; extracts `map_Kd` paths
      in material-declaration order into `texture_paths`; no-op if no `.mtl`
      or no `map_Kd` entries
- [ ] **Task 2.9**: Register `TestMeshIO` in `tests/CMakeLists.txt`; add
      `MeshIO_OBJ.hpp` to installed headers in `CMakeLists.txt`

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All OBJ round-trip tests pass (with and without texture paths)
- [ ] Build succeeds in Debug and Release

---

## Phase 3: PLY IO

Implement `include/educelab/core/io/MeshIO_PLY.hpp` with `read_ply` and
`write_ply`. Implement `expand_at_seams` in `MeshUtils.hpp`. PLY texture
support is single-path only (no vector overload); multi-texture PLY has no
well-supported ecosystem standard.

**Parsing utilities:** use `split()` with no arguments to tokenise ASCII PLY
header and data lines; `to_numeric<T>` for string-to-number conversion;
direct `std::memcpy` / host-endian byte reads for binary-little-endian data.
**Write utilities:** declare one `std::array<char, 128> buf` per write
function and use `to_string_view(buf, val)` for all numeric output — no heap
allocation per conversion.

### Tasks

- [ ] **Task 3.1**: Write tests for `expand_at_seams` in `TestMeshIO.cpp` —
      cover: mesh with no seams (no duplication); mesh with one seam edge
      (verify duplicate vertex count); verify face indices updated correctly;
      verify geometry identical to original at all vertex positions
- [ ] **Task 3.2**: Implement `expand_at_seams(mesh, uvmap) ->
      std::pair<MeshT, std::vector<Vec<T,2>>>` in
      `include/educelab/core/utils/MeshUtils.hpp`; walk each face corner,
      assign UV to vertex if unassigned, duplicate vertex if UV conflicts;
      return expanded mesh and flat per-vertex UV array
- [ ] **Task 3.3**: Write round-trip tests for PLY in `TestMeshIO.cpp` —
      cover: ASCII PLY positions only; positions + normals; positions +
      colors; N-gon faces; binary-little-endian read; `write_ply` with
      `uvmap` (verify vertex count expansion at seams); `write_ply` with
      `uvmap` + single texture path (verify `comment TextureFile` in header
      and round-trip); `read_ply` parsing of `comment TextureFile` into
      `texture_paths`; missing file error
- [ ] **Task 3.4**: Implement `write_ply(path, mesh)` — ASCII PLY; writes
      header with `element vertex` / `element face`; conditionally includes
      `nx ny nz` properties (`if constexpr has_normal<Vertex>`) and
      `red green blue` properties (`if constexpr has_color<Vertex>`); writes
      data section
- [ ] **Task 3.5**: Implement `write_ply(path, mesh, uvmap)` — calls
      `expand_at_seams`, writes `s t` UV properties in vertex header and
      data; no texture comment
- [ ] **Task 3.6**: Implement `write_ply(path, mesh, uvmap, texture_path)` —
      single `std::filesystem::path`; extends Task 3.5; emits one
      `comment TextureFile <path>` line immediately after the `format` line
      in the PLY header (MeshLab convention)
- [ ] **Task 3.7**: Implement `read_ply(path, mesh)` — parses PLY header to
      determine present properties; reads ASCII and binary-little-endian data;
      populates normals/colors via `if constexpr`
- [ ] **Task 3.8**: Implement `read_ply(path, mesh, uvmap, texture_paths)` —
      extends Task 3.7; parses `s t` per-vertex UV properties into `uvmap`
      (note: seam topology is not recovered); parses `comment TextureFile`
      lines from PLY header into `texture_paths` out-parameter (empty if none
      present)
- [ ] **Task 3.9**: Add `MeshIO_PLY.hpp` and updated `MeshUtils.hpp` to
      installed headers in `CMakeLists.txt`

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All PLY round-trip tests pass (with and without texture path)
- [ ] `expand_at_seams` tests pass
- [ ] Build succeeds in Debug and Release

---

## Phase 4: Convenience Facade

Implement `include/educelab/core/io/MeshIO.hpp` — the `read_mesh` /
`write_mesh` convenience wrappers that dispatch by file extension. This header
includes `MeshIO_OBJ.hpp` and `MeshIO_PLY.hpp`; callers who want everything
include only this file.

### Tasks

- [ ] **Task 4.1**: Write tests for `read_mesh` / `write_mesh` in
      `TestMeshIO.cpp` — cover: `.obj` extension dispatches to OBJ functions;
      `.ply` extension dispatches to PLY functions; unsupported extension
      throws; all three tiers (`mesh` only, `mesh + uvmap`,
      `mesh + uvmap + texture_paths` / `texture_path`) compile and dispatch
      correctly
- [ ] **Task 4.2**: Implement `include/educelab/core/io/MeshIO.hpp` —
      includes `MeshIO_OBJ.hpp` and `MeshIO_PLY.hpp`; implements
      `read_mesh(path, mesh)`, `write_mesh(path, mesh)`;
      `read_mesh(path, mesh, uvmap)`, `write_mesh(path, mesh, uvmap)`;
      `read_mesh(path, mesh, uvmap, texture_paths)` (vector out-param);
      `write_mesh(path, mesh, uvmap, texture_path)` (single path);
      dispatches via `path.extension()` comparison; throws
      `std::runtime_error` for unsupported extensions; add `MeshIO.hpp` to
      installed headers in `CMakeLists.txt`

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All convenience dispatch tests pass
- [ ] Build succeeds in Debug and Release

---

## Final Verification

- [ ] All acceptance criteria in `spec.md` met
- [ ] All tests passing (`ctest`)
- [ ] OBJ and PLY round-trips verified for all supported attribute
      combinations including single-texture, multi-chart (OBJ only), and
      no-texture cases
- [ ] Doxygen builds cleanly for all public IO functions and detection traits
- [ ] Ready for PR review

---

_Updated 2026-03-24 to reflect interface design review._

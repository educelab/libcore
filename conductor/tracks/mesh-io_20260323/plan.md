# Implementation Plan: Mesh IO

**Track ID:** mesh-io_20260323
**Spec:** [spec.md](./spec.md)
**Created:** 2026-03-23
**Status:** [ ] Not Started

## Overview

**Blocked on:** `mesh-redesign_20260320` (composable traits) and
`uv-map_20260323` (UVMap + WithChart). Do not begin until both are merged.

Three phases: first establish the detection idiom infrastructure shared by
all IO headers, then implement OBJ IO (including MTL texture path handling),
then PLY IO (including `comment TextureFile` texture path handling and the
seam-expansion adapter).

## Checkpoints

| Phase   | Checkpoint SHA | Date | Status  |
| ------- | -------------- | ---- | ------- |
| Phase 1 |                |      | pending |
| Phase 2 |                |      | pending |
| Phase 3 |                |      | pending |

---

## Phase 1: Trait Detection Infrastructure

Introduce the compile-time detection helpers that OBJ and PLY IO will share.
Keeping them in a `detail/` header avoids duplication and makes the IO
implementations readable.

### Tasks

- [ ] **Task 1.1**: Write tests in `tests/src/TestMeshIO.cpp` that verify
      `has_normal<V>` and `has_color<V>` resolve correctly for vertex types
      with and without those traits (compile-time assertions via
      `static_assert`)
- [ ] **Task 1.2**: Create `include/educelab/core/types/detail/MeshTraits.hpp`
      with `has_normal<T>` and `has_color<T>` detection traits using
      `std::void_t`; document intended use with `if constexpr` in IO functions

### Verification

- [ ] `ctest` passes with no regressions
- [ ] Detection trait `static_assert` tests pass
- [ ] Build succeeds in Debug and Release

---

## Phase 2: OBJ IO

Implement `include/educelab/core/io/MeshIO.hpp` with `readOBJ` and `writeOBJ`,
including MTL texture path read/write.

### Tasks

- [ ] **Task 2.1**: Write round-trip tests for OBJ in `TestMeshIO.cpp` —
      cover: positions only; positions + normals (`WithNormal` mesh); positions
      + colors (`WithColor` mesh); positions + UVs (no texture); positions +
      UVs + single texture path (verify `.mtl` written and path round-trips);
      positions + UVs + multi-chart texture paths via `WithChart` (verify
      per-chart `usemtl` grouping and path recovery); N-gon faces; mesh type
      without normals reading a file that has normals (normals ignored);
      missing file error; `.mtl` present but no `map_Kd` (empty texturePaths)
- [ ] **Task 2.2**: Implement `writeOBJ(path, mesh)` — writes `v` lines;
      conditionally writes `vn` lines (`if constexpr has_normal`); conditionally
      writes `vc` lines (`if constexpr has_color`); writes `f` lines
- [ ] **Task 2.3**: Implement `writeOBJ(path, mesh, uvmap)` — adds `vt` lines
      and per-wedge UV indices in `f` lines; no `.mtl` emitted
- [ ] **Task 2.4**: Implement `writeOBJ(path, mesh, uvmap, texturePath)` —
      single `std::filesystem::path` overload; emits a `.mtl` with one material
      (`newmtl material0`, `map_Kd <path>`); all faces prefixed with
      `usemtl material0`
- [ ] **Task 2.5**: Implement `writeOBJ(path, mesh, uvmap, texturePaths)` —
      `std::vector<std::filesystem::path>` overload; requires `UVMap` with
      `traits::WithChart`; emits one `newmtl materialN` / `map_Kd` entry per
      path; groups faces by chart index of corner 0 with `usemtl materialN`
      directives
- [ ] **Task 2.6**: Implement `readOBJ(path, mesh)` — parses `v`, `vn`, `f`;
      populates normals via `if constexpr`
- [ ] **Task 2.7**: Implement `readOBJ(path, mesh, uvmap)` — also parses `vt`
      and per-wedge UV indices from `f` lines; populates `uvmap`
- [ ] **Task 2.8**: Implement `readOBJ(path, mesh, uvmap, texturePaths)` —
      parses `.mtl` referenced by `mtllib` directive; extracts `map_Kd` paths
      in material-declaration order into `texturePaths`; no-op if no `.mtl` or
      no `map_Kd` entries
- [ ] **Task 2.9**: Register `TestMeshIO` in `tests/CMakeLists.txt`; add
      `MeshIO.hpp` to installed headers in `CMakeLists.txt`

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All OBJ round-trip tests pass (with and without texture paths)
- [ ] Build succeeds in Debug and Release

---

## Phase 3: PLY IO

Extend `MeshIO.hpp` with `readPLY` and `writePLY`, including the
seam-expansion adapter and `comment TextureFile` texture path handling.

### Tasks

- [ ] **Task 3.1**: Write tests for `expandAtSeams` in `TestMeshIO.cpp` —
      cover: mesh with no seams (no duplication), mesh with one seam edge
      (verify duplicate vertex count), verify face indices updated correctly,
      verify geometry identical to original at all vertex positions
- [ ] **Task 3.2**: Implement `expandAtSeams(mesh, uvmap) -> std::pair<MeshT, std::vector<Vec<T,2>>>`
      in `include/educelab/core/utils/MeshUtils.hpp`; walk each face corner,
      assign UV to vertex if unassigned, duplicate vertex if UV conflicts;
      return expanded mesh and flat per-vertex UV array
- [ ] **Task 3.3**: Write round-trip tests for PLY in `TestMeshIO.cpp` —
      cover: ASCII PLY positions only; positions + normals; positions + colors;
      N-gon faces; binary-little-endian read; `writePLY` with `uvmap` (verify
      vertex count expansion at seams); `writePLY` with `uvmap` + single
      texture path (verify `comment TextureFile` in header and round-trip);
      `writePLY` with `uvmap` + multiple texture paths (verify all
      `comment TextureFile` lines written in order); `readPLY` parsing of
      `comment TextureFile` into `texturePaths`; missing file error
- [ ] **Task 3.4**: Implement `writePLY(path, mesh)` — ASCII PLY; writes
      header with `element vertex` / `element face`; conditionally includes
      `nx ny nz` properties (`if constexpr has_normal`) and `red green blue`
      properties (`if constexpr has_color`); writes data section
- [ ] **Task 3.5**: Implement `writePLY(path, mesh, uvmap)` — calls
      `expandAtSeams`, writes `s t` UV properties in vertex header and data;
      no texture comment
- [ ] **Task 3.6**: Implement `writePLY(path, mesh, uvmap, texturePaths)` —
      extends Task 3.5; emits one `comment TextureFile <path>` line per entry
      in `texturePaths` immediately after the `format` line in the PLY header
      (MeshLab convention)
- [ ] **Task 3.7**: Implement `readPLY(path, mesh)` — parses PLY header to
      determine present properties; reads ASCII and binary-little-endian data;
      populates normals/colors via `if constexpr`
- [ ] **Task 3.8**: Implement `readPLY(path, mesh, texturePaths)` — extends
      Task 3.7; parses `comment TextureFile` lines from PLY header into
      `texturePaths` out-parameter (empty if none present)

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All PLY round-trip tests pass (with and without texture paths)
- [ ] Build succeeds in Debug and Release

---

## Final Verification

- [ ] All acceptance criteria in `spec.md` met
- [ ] All tests passing (`ctest`)
- [ ] OBJ and PLY round-trips verified for all supported attribute combinations
      including single-texture, multi-chart texture, and no-texture cases
- [ ] Doxygen builds cleanly for all public IO functions
- [ ] Ready for PR review

---

_Generated by Conductor. Tasks will be marked [~] in progress and [x] complete._

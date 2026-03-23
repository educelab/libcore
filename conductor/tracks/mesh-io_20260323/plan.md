# Implementation Plan: Mesh IO

**Track ID:** mesh-io_20260323
**Spec:** [spec.md](./spec.md)
**Created:** 2026-03-23
**Status:** [ ] Not Started

## Overview

**Blocked on:** `mesh-redesign_20260320` (composable traits) and
`uv-map_20260323` (UVMap). Do not begin until both are merged.

Three phases: first establish the detection idiom infrastructure shared by
all IO headers, then implement OBJ IO, then PLY IO.

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

Implement `include/educelab/core/io/MeshIO.hpp` with `readOBJ` and `writeOBJ`.

### Tasks

- [ ] **Task 2.1**: Write round-trip tests for OBJ in `TestMeshIO.cpp` —
      cover: positions only, positions + normals (`WithNormal` mesh), positions
      + colors (`WithColor` mesh), positions + UVs (with `UVMap`), N-gon faces,
      mesh type without normals reading a file that has normals (normals
      ignored), missing file error
- [ ] **Task 2.2**: Implement `writeOBJ(path, mesh)` — writes `v` lines;
      conditionally writes `vn` lines (`if constexpr has_normal`); conditionally
      writes `vc` lines (`if constexpr has_color`); writes `f` lines with
      appropriate index syntax
- [ ] **Task 2.3**: Implement `writeOBJ(path, mesh, uvmap)` overload — adds
      `vt` lines and per-wedge UV indices in `f` lines; writes minimal `.mtl`
      referencing a texture path if provided
- [ ] **Task 2.4**: Implement `readOBJ(path, mesh)` — parses `v`, `vn`, `f`;
      populates normals via `if constexpr`; returns optional texture path from
      `.mtl` if present
- [ ] **Task 2.5**: Implement `readOBJ(path, mesh, uvmap)` overload — also
      parses `vt` and per-wedge UV indices from `f` lines; populates `uvmap`
- [ ] **Task 2.6**: Register `TestMeshIO` in `tests/CMakeLists.txt`; add
      `MeshIO.hpp` to installed headers in `CMakeLists.txt`

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All OBJ round-trip tests pass
- [ ] Build succeeds in Debug and Release

---

## Phase 3: PLY IO

Extend `MeshIO.hpp` with `readPLY` and `writePLY`.

### Tasks

- [ ] **Task 3.1**: Write tests for `expandAtSeams` in `TestMeshIO.cpp` —
      cover: mesh with no seams (no duplication), mesh with one seam edge
      (verify duplicate vertex count), verify face indices updated correctly,
      verify geometry identical to original at all vertex positions
- [ ] **Task 3.2**: Implement `expandAtSeams(mesh, uvmap) -> std::pair<MeshT, std::vector<Vec<float,2>>>`
      in `include/educelab/core/utils/MeshUtils.hpp`; walk each face corner,
      assign UV to vertex if unassigned, duplicate vertex if UV conflicts;
      return expanded mesh and flat per-vertex UV array
- [ ] **Task 3.3**: Write round-trip tests for PLY in `TestMeshIO.cpp` —
      cover: ASCII PLY positions only, positions + normals, positions + colors,
      N-gon faces, binary-little-endian read, `writePLY` with `uvmap` (verify
      vertex count expansion at seams), missing file error
- [ ] **Task 3.4**: Implement `writePLY(path, mesh)` — ASCII PLY; writes
      header with `element vertex` / `element face`; conditionally includes
      `nx ny nz` properties (`if constexpr has_normal`) and `red green blue`
      properties (`if constexpr has_color`); writes data section
- [ ] **Task 3.5**: Implement `writePLY(path, mesh, uvmap)` overload — calls
      `expandAtSeams`, writes `s t` UV properties in vertex header and data
- [ ] **Task 3.6**: Implement `readPLY(path, mesh)` — parses PLY header to
      determine present properties; reads ASCII and binary-little-endian data;
      populates normals/colors via `if constexpr`

### Verification

- [ ] `ctest` passes with no regressions
- [ ] All PLY round-trip tests pass
- [ ] Build succeeds in Debug and Release

---

## Final Verification

- [ ] All acceptance criteria in `spec.md` met
- [ ] All tests passing (`ctest`)
- [ ] OBJ and PLY round-trips verified for all supported attribute combinations
- [ ] Doxygen builds cleanly for all public IO functions
- [ ] Ready for PR review

---

_Generated by Conductor. Tasks will be marked [~] in progress and [x] complete._

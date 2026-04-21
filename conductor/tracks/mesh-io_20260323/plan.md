# Implementation Plan: Mesh IO

**Track ID:** mesh-io_20260323
**Spec:** [spec.md](./spec.md)
**Created:** 2026-03-23
**Updated:** 2026-03-25
**Status:** [x] Complete

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
| Phase 2 | 0a4efdd        | 2026-03-25 | complete |
| Phase 3 | ea14fbf        | 2026-03-25 | complete |
| Phase 4 | 1d0e3fc        | 2026-03-25 | complete |

---

## Phase 1: Trait Detection Infrastructure

Introduce the compile-time detection helpers that OBJ and PLY IO will share.
Mesh traits (`has_normal`, `has_color`) live in `Mesh.hpp`. UVMap traits 
(`has_chart`) live in `UVMap.hpp` and must be documented with opt-in examples.

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

- [x] **Task 2.1**: Write round-trip tests for OBJ in `TestMeshIO.cpp` —
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
- [x] **Task 2.2**: Implement `write_obj(path, mesh)` — writes `v` lines with
      optional inline RGB (`if constexpr has_color<Vertex>`); writes `vn`
      lines (`if constexpr has_normal<Vertex>`); writes `f` lines `0a4efdd`
- [x] **Task 2.3**: Implement `write_obj(path, mesh, uvmap)` — adds `vt`
      lines and per-wedge UV indices in `f` lines; no `.mtl` emitted `0a4efdd`
- [x] **Task 2.4**: Implement `write_obj(path, mesh, uvmap, texture_path)` —
      single `std::filesystem::path`; emits a `.mtl` with one material
      (`newmtl material0`, `map_Kd <path>`); all faces prefixed with
      `usemtl material0` `0a4efdd`
- [x] **Task 2.5**: Implement `write_obj(path, mesh, uvmap, texture_paths)` —
      `std::vector<std::filesystem::path>` overload;
      `static_assert(traits::has_chart<UVMapT>::value, "write_obj with multiple
      texture paths requires UVMap with traits::WithChart")`; emits one
      `newmtl materialN` / `map_Kd` entry per path; groups faces by the chart
      index of corner 0 with `usemtl materialN` directives `0a4efdd`
- [x] **Task 2.6**: Implement `read_obj(path, mesh)` — parses `v` lines with
      3 or 6+ components (position only vs position + RGB); parses `vn`, `f`;
      populates normals via `if constexpr has_normal<Vertex>`; populates
      colors via `if constexpr has_color<Vertex>`; silently skips
      unrecognized directives `0a4efdd`
- [x] **Task 2.7**: Implement `read_obj(path, mesh, uvmap)` — also parses
      `vt` and per-wedge UV indices from `f` lines; populates `uvmap`;
      populates chart indices on `uvmap` coordinates via
      `if constexpr traits::has_chart<UVMapT>` using material group ordering `0a4efdd`
- [x] **Task 2.8**: Implement `read_obj(path, mesh, uvmap, texture_paths)` —
      parses `.mtl` referenced by `mtllib` directive; extracts `map_Kd` paths
      in material-declaration order into `texture_paths`; no-op if no `.mtl`
      or no `map_Kd` entries `0a4efdd`
- [x] **Task 2.9**: Register `TestMeshIO` in `tests/CMakeLists.txt`; add
      `MeshIO_OBJ.hpp` to installed headers in `CMakeLists.txt` `0a4efdd`

### Verification

- [x] `ctest` passes with no regressions
- [x] All OBJ round-trip tests pass (with and without texture paths)
- [x] Build succeeds in Debug and Release

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

- [x] **Task 3.1**: Write tests for `expand_at_seams` in `TestMeshIO.cpp` —
      cover: mesh with no seams (no duplication); mesh with one seam edge
      (verify duplicate vertex count); verify face indices updated correctly;
      verify geometry identical to original at all vertex positions `ea14fbf`
- [x] **Task 3.2**: Implement `expand_at_seams(mesh, uvmap) ->
      std::pair<MeshT, std::vector<Vec<T,2>>>` in
      `include/educelab/core/utils/MeshUtils.hpp`; walk each face corner,
      assign UV to vertex if unassigned, duplicate vertex if UV conflicts;
      return expanded mesh and flat per-vertex UV array `ea14fbf`
- [x] **Task 3.3**: Write round-trip tests for PLY in `TestMeshIO.cpp` —
      cover: ASCII PLY positions only; positions + normals; positions +
      colors; N-gon faces; binary-little-endian read; `write_ply` with
      `uvmap` (verify vertex count expansion at seams); `write_ply` with
      `uvmap` + single texture path (verify `comment TextureFile` in header
      and round-trip); `read_ply` parsing of `comment TextureFile` into
      `texture_paths`; missing file error `ea14fbf`
- [x] **Task 3.4**: Implement `write_ply(path, mesh)` — ASCII PLY; writes
      header with `element vertex` / `element face`; conditionally includes
      `nx ny nz` properties (`if constexpr has_normal<Vertex>`) and
      `red green blue` properties (`if constexpr has_color<Vertex>`); writes
      data section `ea14fbf`
- [x] **Task 3.5**: Implement `write_ply(path, mesh, uvmap)` — calls
      `expand_at_seams`, writes `s t` UV properties in vertex header and
      data; no texture comment `ea14fbf`
- [x] **Task 3.6**: Implement `write_ply(path, mesh, uvmap, texture_path)` —
      single `std::filesystem::path`; extends Task 3.5; emits one
      `comment TextureFile <path>` line immediately after the `format` line
      in the PLY header (MeshLab convention) `ea14fbf`
- [x] **Task 3.7**: Implement `read_ply(path, mesh)` — parses PLY header to
      determine present properties; reads ASCII and binary-little-endian data;
      populates normals/colors via `if constexpr` `ea14fbf`
- [x] **Task 3.8**: Implement `read_ply(path, mesh, uvmap, texture_paths)` —
      extends Task 3.7; parses `s t` per-vertex UV properties into `uvmap`
      (note: seam topology is not recovered); parses `comment TextureFile`
      lines from PLY header into `texture_paths` out-parameter (empty if none
      present) `ea14fbf`
- [x] **Task 3.9**: Add `MeshIO_PLY.hpp` and updated `MeshUtils.hpp` to
      installed headers in `CMakeLists.txt` `ea14fbf`

### Verification

- [x] `ctest` passes with no regressions
- [x] All PLY round-trip tests pass (with and without texture path)
- [x] `expand_at_seams` tests pass
- [x] Build succeeds in Debug and Release

---

## Phase 4: Convenience Facade

Implement `include/educelab/core/io/MeshIO.hpp` — the `read_mesh` /
`write_mesh` convenience wrappers that dispatch by file extension. This header
includes `MeshIO_OBJ.hpp` and `MeshIO_PLY.hpp`; callers who want everything
include only this file.

### Tasks

- [x] **Task 4.1**: Write tests for `read_mesh` / `write_mesh` in
      `TestMeshIO.cpp` — cover: `.obj` extension dispatches to OBJ functions;
      `.ply` extension dispatches to PLY functions; unsupported extension
      throws; all three tiers (`mesh` only, `mesh + uvmap`,
      `mesh + uvmap + texture_paths` / `texture_path`) compile and dispatch
      correctly `1d0e3fc`
- [x] **Task 4.2**: Implement `include/educelab/core/io/MeshIO.hpp` —
      includes `MeshIO_OBJ.hpp` and `MeshIO_PLY.hpp`; implements
      `read_mesh(path, mesh)`, `write_mesh(path, mesh)`;
      `read_mesh(path, mesh, uvmap)`, `write_mesh(path, mesh, uvmap)`;
      `read_mesh(path, mesh, uvmap, texture_paths)` (vector out-param);
      `write_mesh(path, mesh, uvmap, texture_path)` (single path);
      dispatches via `path.extension()` comparison; throws
      `std::runtime_error` for unsupported extensions; add `MeshIO.hpp` to
      installed headers in `CMakeLists.txt` `1d0e3fc`

### Verification

- [x] `ctest` passes with no regressions
- [x] All convenience dispatch tests pass
- [x] Build succeeds in Debug and Release

---

## Phase 5: Review Fixes and Binary PLY Performance

Address findings from the multi-dimension code review (security, performance,
architecture, testing) plus deferred binary PLY reader optimisations.

### Tasks

- [x] **Task 5.1**: Fix `to_chars<long double>` linker failure on macOS-26 —
      add `long double` probe to `CheckCharconvFP.cmake`; add
      `to_string_view` specialisation for `long double` that casts to
      `double` when `EDUCE_CORE_NEED_CHARCONV_FP_LONG_DOUBLE` is defined
- [x] **Task 5.2**: Security fixes —
      (a) `parse_face_ref`: throw `std::runtime_error` on negative or zero
      OBJ face index (check for leading `-` before `to_numeric`, check
      result == 0 before subtract);
      (b) PLY per-face vertex count: cap at 256, throw if exceeded (both
      ASCII and binary paths);
      (c) PLY face indices: validate each index against `hdr.n_vertices`
      in `read_ply_impl`, throw if out of range;
      (d) BinaryBE: throw `std::runtime_error` instead of falling through
      to ASCII path
- [x] **Task 5.3**: Performance fixes —
      (a) `split()`: use `thread_local static std::locale` instead of
      constructing on every call;
      (b) add `split_into(sv, vec)` overload that clears and reuses a
      caller-provided vector; update OBJ and PLY ASCII parsers to use it;
      (c) `read_obj_impl`: hoist `face_vts`, `face_vns`, `face_verts`
      declarations above the face-parsing loop;
      (d) `expand_at_seams`: replace `std::map` with `std::unordered_map`
      using an inline documented hash lambda for
      `std::pair<std::size_t, std::size_t>`
- [x] **Task 5.4**: Architecture fixes —
      (a) `detail::write_obj_impl(file, mesh, uvmap*, texture_path*)`
      consolidating Tiers 1–3a; Tier 3b reuses vertex-writing sub-helper;
      (b) `detail::write_ply_impl(file, exp_mesh, flat_uvs*, texture_path*)`
      consolidating all three PLY write tiers;
      (c) add `read_ply(path, mesh, uvmap)` Tier 2 overload (delegates to
      `detail::read_ply_impl` with `nullptr` texture_paths);
      (d) fix `read_mesh(path, mesh, uvmap)` facade to call the new Tier 2
      `read_ply` directly instead of constructing a throwaway vector
- [x] **Task 5.5**: Testing fixes —
      (a) unique temp dirs: use `this` pointer address as hex suffix in all
      three fixtures;
      (b) verify all 9 position components in OBJ/PLY position-only
      round-trip tests;
      (c) verify all three vertex colors in `PositionsWithColors` tests;
      (d) add write-failure tests for `write_obj`, `write_ply`, and
      `write_mesh` on an unwritable path;
      (e) add PLY UV coordinate value verification (not just count);
      (f) add `v/vt/vn` combined face format OBJ test;
      (g) add PLY malformed/truncated header test;
      (h) add `NCMesh` (normals + colors combined) OBJ round-trip test;
      (i) add BinaryBE throw test;
      (j) add `expand_at_seams` empty-mesh test
- [ ] **Task 5.6** *(deferred perf)*: Pre-compute vertex AND face property role
      enums from PLY header; replace per-property string comparison in both
      binary AND ASCII reader inner loops with role-indexed dispatch (currently
      `prop.name == "x"` etc. runs for every vertex property on every vertex in
      ASCII too)
- [ ] **Task 5.7** *(deferred perf)*: Batch-read full vertex record into a
      stack buffer per vertex in binary PLY reader; extract fields with
      `std::memcpy` to reduce `istream::read` calls from
      O(properties × vertices) to O(vertices)
- [ ] **Task 5.8** *(deferred refactor)*: Extract `read_ply_face_binary` and
      `read_ply_face_ascii` helpers from `read_ply_impl` to reduce nesting depth
      and isolate the per-face parsing logic; the face-element branch currently
      contains ~80 lines of nested binary+ASCII code inside the element loop

### Verification

- [x] `ctest` passes with no regressions (17/17)
- [x] All new tests pass
- [x] Build succeeds on macOS locally; macOS-26 long double fix in place

---

## Phase 6: PLY Texcoord Approach

Replace per-vertex `s`/`t` scalar UV with the per-wedge
`property list uchar float texcoord` face list. Removes seam expansion from
write path; adds backward-compat read for legacy `s`/`t` files.

Completed changes (texcoord implementation):
- `write_ply_header`: `s`/`t` vertex properties → `property list uchar float texcoord` on face element
- `write_ply_data`: writes `-1 -1` sentinel for unmapped corners; removed `expand_at_seams` call
- `read_ply_impl`: detects `has_texcoord` flag; `legacy_st_uvs` const bool guards old path; `reserve_faces` before face loop; hoisted `fline`; inlined UV insert into vertex loop (removed `uv_s`/`uv_t` temp vectors + second pass)
- Tests: `WriteWithUVMap_SeamExpansion` → `WriteWithUVMap_PerWedgeTexcoord` (verifies no expansion, 6 pool entries, per-wedge values); added `ReadLegacyPerVertexUV_BackwardCompat`

### Tasks

- [x] **Task 6.1**: Implement texcoord write/read in `MeshIO_PLY.hpp`; update tests
- [x] **Task 6.2**: Apply quick-fix review findings (section banners, docstrings, `v_coord`→`v`, static_cast, comments)
- [ ] **Task 5.6** *(carry-forward — see above)*: Role-enum dispatch for binary + ASCII inner loops
- [ ] **Task 5.7** *(carry-forward — see above)*: Batch vertex-record reads
- [ ] **Task 5.8** *(carry-forward — see above)*: Extract face-parsing helpers

- [x] **Task 6.3** *(refactor)*: Replace manual `\r`-strip and empty-check idiom in
      `MeshIO_PLY.hpp` ASCII read paths with `trim_right_in_place` from
      `String.hpp`. The pattern `if (!line.empty() && line.back() == '\r') line.pop_back()` (plus the
      subsequent `if (!line.empty() && line.front() != '#')` guard) appears in
      three places — `skip_ascii_line`, the vertex ASCII read loop, and the face ASCII
      read loop — and can each be collapsed to a single `trim_right_in_place(line)`
      call before the `#`-comment check. No behavior change; `trim_right` already
      treats `\r` as whitespace via `std::isspace`.

### Verification

- [x] `ctest` 17/17 passes
- [x] Build succeeds in Debug

---

## Final Verification

- [x] All acceptance criteria in `spec.md` met
- [x] All tests passing (`ctest`)
- [x] OBJ and PLY round-trips verified for all supported attribute
      combinations including single-texture, multi-chart (OBJ only), and
      no-texture cases
- [ ] Doxygen builds cleanly for all public IO functions and detection traits
- [x] Ready for PR review

---

_Updated 2026-03-25 to add Phase 5 review-fix and deferred binary PLY perf tasks._
_Updated 2026-04-21 to add Phase 6 PLY texcoord approach (per-wedge UV, backward-compat s/t read); expanded Task 5.6 scope to cover ASCII; added Task 5.8 face-helper refactor._

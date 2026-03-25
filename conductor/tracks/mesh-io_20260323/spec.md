# Specification: Mesh IO

**Track ID:** mesh-io_20260323
**Type:** Feature
**Created:** 2026-03-23
**Status:** Draft

## Summary

Provide OBJ and PLY reader/writer functions for `Mesh`, supporting vertex
normals, vertex colors, N-gon faces, and per-chart texture image paths. Readers
and writers use `if constexpr` + the C++17 detection idiom to silently skip
attributes the target mesh type does not carry. Texture image loading is left
to the caller; the IO layer manages only paths.

## Context

EduceLab Core provides common types for EduceLab C++ projects. Downstream
projects (volume-cartographer, registration-toolkit) currently use ITK/VTK for
mesh IO and OpenCV/libtiff for texture loading; providing OBJ and PLY support
in libcore is a prerequisite to replacing those dependencies. The mesh IO layer
must compose cleanly with the `Mesh` composable traits system (`WithNormal`,
`WithColor`) from `mesh-redesign_20260320` and the `UVMap` companion type
from `uv-map_20260323`.

Texture paths are passed separately from `UVMap` as a
`std::vector<std::filesystem::path>` indexed by `traits::WithChart::chart`.
Chart 0 is the implicit single-texture case; a `UVMap` without `WithChart`
(all coordinates at the default `chart=0`) uses index 0. Image loading is
always the caller's responsibility via `ImageIO`.

## User Story

As an EduceLab developer, I want to read and write OBJ and PLY mesh files
(including UV coordinates and texture image paths) so that I can load and save
textured meshes without depending on ITK, VTK, or OpenCV.

## Acceptance Criteria

- [ ] `readOBJ(path, mesh, uvmap, texturePaths)` and
      `writeOBJ(path, mesh, uvmap, texturePaths)` support: vertex positions,
      N-gon faces, per-wedge UV coordinates (via `UVMap`), vertex normals (if
      `WithNormal` trait present), vertex colors (if `WithColor` trait present);
      unrecognized OBJ directives are silently skipped
- [ ] `writeOBJ` `texturePaths` overloads:
      - `writeOBJ(path, mesh, uvmap)` — no texture; no `.mtl` emitted
      - `writeOBJ(path, mesh, uvmap, texturePath)` — single
        `std::filesystem::path`; emits a `.mtl` with one material (`map_Kd`)
        regardless of `UVMap` traits; all faces reference that material
      - `writeOBJ(path, mesh, uvmap, texturePaths)` — `std::vector<std::filesystem::path>`;
        requires `UVMap` with `traits::WithChart`; emits one material per chart
        in the `.mtl`, groups faces by chart using `usemtl` directives
- [ ] `readOBJ` populates a `std::vector<std::filesystem::path>` out-parameter
      with all `map_Kd` paths from the referenced `.mtl` (empty if no `.mtl`
      or no `map_Kd` entries); order matches chart index (material 0 → index 0)
- [ ] `readPLY(path, mesh)` and `writePLY(path, mesh)` support: vertex
      positions, N-gon faces, vertex normals (if `WithNormal` trait present),
      vertex colors (if `WithColor` trait present)
- [ ] `writePLY(path, mesh, uvmap)` and `writePLY(path, mesh, uvmap, texturePaths)`
      are supported via a seam-expansion adapter: vertices at UV seam edges are
      duplicated so each vertex carries a unique UV coordinate; the expanded
      mesh is written, not the original; round-tripping through PLY with UVs
      does not recover the original vertex count or seam topology (documented)
- [ ] `writePLY` with `texturePaths` writes one `comment TextureFile <path>`
      line per path in the PLY header (MeshLab convention), in chart-index
      order; `readPLY` parses these comments into a
      `std::vector<std::filesystem::path>` out-parameter
- [ ] Readers/writers are function templates over `MeshT`; absent traits are
      detected via `std::void_t` and handled with `if constexpr` — no runtime
      branching or virtual dispatch
- [ ] All reader functions return `bool` (or throw on error) and populate the
      mesh in-place; error reporting is consistent with existing `ImageIO`
      convention
- [ ] Tests cover: round-trip write+read for OBJ and PLY, meshes with and
      without normals/colors/UVs/texture paths, single-texture and multi-chart
      texture paths, N-gon faces, missing/malformed files
- [ ] All additions are C++17 compatible; IO headers live under `io/`

## Dependencies

- `mesh-redesign_20260320` — composable traits (`WithNormal`, `WithColor`),
  adjacency index, and face normal cache must be complete and merged
- `uv-map_20260323` — `UVMap` (including `traits::WithChart`) must be complete
  and merged
- `ImageIO.hpp` — existing; used by callers to load texture images; not a new
  dependency for this track
- `<filesystem>` — C++17 standard; no new external dependency

## Out of Scope

- Multiple UV channels per mesh (a single `UVMap` per mesh)
- Binary PLY write (ASCII PLY only; binary-little-endian read is in scope)
- STL read/write (future feature)
- glTF / FBX / other formats
- Texture image loading inside IO functions (callers use `ImageIO`)
- Mesh validation on read
- Full MTL material support (only `map_Kd` diffuse texture path extracted)

## Technical Notes

### Texture Path Convention

`texturePaths[i]` is the image for chart `i`. A `UVMap` without `WithChart`
(all coordinates default to `chart=0`) uses `texturePaths[0]`. Callers
building single-texture meshes from VC/RT do not need `WithChart` — they pass
a single path via the `std::filesystem::path` overload or a one-element vector.

### OBJ / MTL Multi-Texture Layout

Single texture (one path):
```
# mesh.mtl
newmtl material0
map_Kd texture.png
```
All `f` lines are preceded by a single `usemtl material0`.

Multi-texture (chart-indexed paths):
```
# mesh.mtl
newmtl material0
map_Kd chart0.png

newmtl material1
map_Kd chart1.png
```
`writeOBJ` groups faces by the chart index of their corner 0 vertex and emits
`usemtl materialN` before each group. `readOBJ` maps material declaration order
to chart index (first material → chart 0).

Only `map_Kd` is read/written; all other MTL directives are ignored on read
and not emitted on write.

### PLY Texture Header Comments (MeshLab convention)

```
comment TextureFile chart0.png
comment TextureFile chart1.png
```

One `comment TextureFile` line per chart, in chart-index order, in the PLY
header immediately after the `ply` / `format` lines. This is the format
written by MeshLab and readable by Blender, CloudCompare, and other tools.
`readPLY` parses these comments into the `texturePaths` out-parameter.

### Detection Idiom Infrastructure

`has_normal<T>` and `has_color<T>` traits live in
`include/educelab/core/types/detail/MeshTraits.hpp`, shared between `Mesh.hpp`
and all IO headers.

### Seam-Expansion Adapter

`expandAtSeams(mesh, uvmap) -> std::pair<MeshT, std::vector<Vec<T,2>>>`
lives in `include/educelab/core/utils/MeshUtils.hpp`. `writePLY` with a
`uvmap` calls it internally. The returned flat per-vertex UV array is written
as `s t` properties. This adapter is independently useful for GPU upload.

---

_Generated by Conductor. Review and edit as needed._

# Specification: Mesh IO

**Track ID:** mesh-io_20260323
**Type:** Feature
**Created:** 2026-03-23
**Updated:** 2026-03-24
**Status:** Draft

## Summary

Provide OBJ and PLY reader/writer functions for `Mesh`, supporting vertex
normals, vertex colors, N-gon faces, per-wedge UV coordinates, and texture
image paths. The library exposes two layers:

- **Format-specific functions** (`read_obj`, `write_obj`, `read_ply`,
  `write_ply`) for callers who know their format and need full control over
  format-specific options.
- **Convenience facade** (`read_mesh`, `write_mesh` in `MeshIO.hpp`) that
  dispatches to the format-specific functions by file extension, covering the
  common single-texture and no-texture cases.

Readers and writers use `if constexpr` + the C++17 detection idiom to
silently skip attributes the target mesh or UV-map type does not carry.
Texture image loading is left to the caller; the IO layer manages paths only.
All errors are reported by throwing exceptions (consistent with `write_image`).

## Context

EduceLab Core provides common types for EduceLab C++ projects. Downstream
projects (volume-cartographer, registration-toolkit) currently use ITK/VTK for
mesh IO; providing OBJ and PLY support in libcore is a prerequisite to
replacing those dependencies. The mesh IO layer composes with the `Mesh`
composable traits system (`WithNormal`, `WithColor`) from
`mesh-redesign_20260320` and the `UVMap` companion type from
`uv-map_20260323`.

Texture paths are passed separately from `UVMap` as a
`std::vector<std::filesystem::path>` indexed by chart. Chart 0 is the
implicit single-texture case. Image loading is always the caller's
responsibility via `ImageIO`.

## User Story

As an EduceLab developer, I want to read and write OBJ and PLY mesh files
(including UV coordinates and texture image paths) so that I can load and save
textured meshes without depending on ITK, VTK, or OpenCV.

## Acceptance Criteria

### Format-specific functions

- [ ] `read_obj` / `write_obj` and `read_ply` / `write_ply` are function
      templates over `MeshT`; absent vertex traits (`WithNormal`, `WithColor`)
      are detected at compile time via `has_normal<V>` / `has_color<V>` and
      handled with `if constexpr` — no runtime branching or virtual dispatch
- [ ] `read_obj(path, mesh)` / `write_obj(path, mesh)`: vertex positions,
      N-gon faces; vertex normals written/read if `has_normal<Vertex>`;
      vertex colors written/read as inline RGB on `v` lines
      (`v x y z r g b`) if `has_color<Vertex>`; unrecognized OBJ directives
      silently skipped on read
- [ ] `read_obj(path, mesh, uvmap)` / `write_obj(path, mesh, uvmap)`:
      additionally handles per-wedge UV coordinates via `UVMap`; on read,
      chart indices are populated on `uvmap` coordinates if
      `has_chart<UVMapT>` (detected via the detection idiom), otherwise
      silently skipped
- [ ] `read_obj(path, mesh, uvmap, texture_paths)`: populates `texture_paths`
      (a `std::vector<std::filesystem::path>` out-parameter) with all `map_Kd`
      paths from the referenced `.mtl` in material-declaration order (empty if
      no `.mtl` or no `map_Kd` entries); first material → `texture_paths[0]`
- [ ] `write_obj(path, mesh, uvmap, texture_path)`: single
      `std::filesystem::path`; emits a `.mtl` with one material
      (`newmtl material0`, `map_Kd <path>`); all faces reference that material
- [ ] `write_obj(path, mesh, uvmap, texture_paths)`:
      `std::vector<std::filesystem::path>`; requires `UVMap` with
      `traits::WithChart` — enforced via
      `static_assert(traits::has_chart<UVMapT>::value, ...)`; emits one
      `newmtl materialN` / `map_Kd` entry per path; groups faces by the chart
      index of corner 0 with `usemtl materialN` directives
- [ ] `read_ply(path, mesh)` / `write_ply(path, mesh)`: vertex positions,
      N-gon faces; vertex normals and colors handled via `if constexpr` as
      above; ASCII and binary-little-endian PLY files supported on read;
      write produces ASCII PLY
- [ ] `read_ply(path, mesh, uvmap)` / `write_ply(path, mesh, uvmap)`:
      additionally handles per-wedge UV coordinates; `write_ply` with a
      `uvmap` calls `expand_at_seams` internally — vertices at UV seam edges
      are duplicated so each vertex carries a unique UV; the expanded mesh is
      written; round-tripping through PLY with UVs does not recover the
      original vertex count (documented)
- [ ] `read_ply(path, mesh, uvmap, texture_paths)` /
      `write_ply(path, mesh, uvmap, texture_path)`: single texture path only
      for PLY (no vector overload — multi-texture PLY has no well-supported
      ecosystem standard); `write_ply` emits one `comment TextureFile <path>`
      line in the PLY header (MeshLab convention); `read_ply` parses
      `comment TextureFile` lines into `texture_paths`

### Convenience facade

- [ ] `read_mesh(path, mesh)` / `write_mesh(path, mesh)`: dispatch to
      `read_obj` / `write_obj` or `read_ply` / `write_ply` by file extension;
      throw `std::runtime_error` for unsupported extensions
- [ ] `read_mesh(path, mesh, uvmap)` / `write_mesh(path, mesh, uvmap)`:
      as above, with UV map
- [ ] `read_mesh(path, mesh, uvmap, texture_paths)`: as above; `texture_paths`
      is `std::vector<std::filesystem::path>&` out-parameter, populated with
      all paths the format provides (0 or 1 for PLY, 0–N for OBJ)
- [ ] `write_mesh(path, mesh, uvmap, texture_path)`: single path; dispatches
      to the single-path overload of the format-specific writer

### General

- [ ] All functions throw on error (file-not-found, write failure, malformed
      input) — consistent with `write_image`
- [ ] Tests cover: round-trip write+read for OBJ and PLY; meshes with and
      without normals/colors/UVs/texture paths; single-texture and multi-chart
      OBJ texture paths; `static_assert` fires for multi-chart `write_obj`
      without `WithChart` (verified by compile-fail test or comment);
      N-gon faces; missing/malformed files; convenience `read_mesh` /
      `write_mesh` extension dispatch
- [ ] All additions are C++17 compatible; IO headers live under `io/`

## Dependencies

- `mesh-redesign_20260320` — composable traits (`WithNormal`, `WithColor`),
  adjacency index, and face normal cache — **complete**
- `uv-map_20260323` — `UVMap` (including `traits::WithChart`) — **complete**
- `ImageIO.hpp` — existing; used by callers to load texture images; not a new
  dependency for this track
- `<filesystem>` — C++17 standard; no new external dependency

## Out of Scope

- Multiple UV channels per mesh (single `UVMap` per mesh)
- Binary PLY write (ASCII PLY only; binary-little-endian read is in scope)
- Multi-texture PLY write (no well-supported standard outside MeshLab)
- STL, glTF, FBX, or other formats
- Texture image loading inside IO functions (callers use `ImageIO`)
- Mesh validation on read
- Full MTL material support (only `map_Kd` diffuse texture path extracted)

## Technical Notes

### Header Layout

```
include/educelab/core/io/MeshIO.hpp       ← convenience facade (read_mesh, write_mesh)
                                            includes MeshIO_OBJ.hpp + MeshIO_PLY.hpp
include/educelab/core/io/MeshIO_OBJ.hpp  ← read_obj, write_obj
include/educelab/core/io/MeshIO_PLY.hpp  ← read_ply, write_ply
```

Callers who need only one format can include `MeshIO_OBJ.hpp` or
`MeshIO_PLY.hpp` directly. Callers who want format-agnostic access include
`MeshIO.hpp`.

### Detection Idiom Infrastructure

`has_normal<V>`, `has_color<V>`, and `traits::has_chart<UVMapT>` live in
their respective type headers (`Mesh.hpp` and `UVMap.hpp`), shared between
`Mesh.hpp` and all IO headers.

- `has_normal<V>` — true if `V` inherits `traits::WithNormal`
- `has_color<V>` — true if `V` inherits `traits::WithColor`
- `traits::has_chart<UVMapT>` — true if `UVMapT::Coordinate` inherits
  `traits::WithChart`; used by `read_obj` to conditionally populate chart
  indices and by `write_obj` multi-path overload (`static_assert`)

All three traits must be clearly documented with an example showing the
opt-in pattern and explaining that IO functions use them via `if constexpr`.

### OBJ Vertex Color Convention

Vertex colors are written inline on `v` lines as three additional float
components after the position:

```
v 1.0 2.0 3.0 0.8 0.2 0.4
```

This is the convention used by MeshLab, Blender, and CloudCompare. On read,
a `v` line with 6 or 7 components is treated as position + RGB (or position +
RGB + W); fewer components are position only. The separate `vc` directive is
not used.

### OBJ / MTL Multi-Texture Layout

Single texture (one path):
```
# mesh.mtl
newmtl material0
map_Kd texture.png
```
All `f` lines are preceded by a single `usemtl material0`.

Multi-texture (chart-indexed paths, `write_obj` with `vector<path>` only):
```
# mesh.mtl
newmtl material0
map_Kd chart0.png

newmtl material1
map_Kd chart1.png
```
`write_obj` groups faces by the chart index of their corner 0 vertex and
emits `usemtl materialN` before each group. `read_obj` maps material
declaration order to chart index (first material → chart 0); chart indices
are written to `uvmap` coordinates only if `traits::has_chart<UVMapT>`.

Only `map_Kd` is read/written; all other MTL directives are ignored on read
and not emitted on write.

### PLY Texture Header Comments (MeshLab convention)

```
ply
format ascii 1.0
comment TextureFile texture.png
```

One `comment TextureFile` line per texture path, in chart-index order,
immediately after the `format` line. This is the format written by MeshLab
and readable by Blender, CloudCompare, and other tools. PLY supports only a
single texture path via the convenience facade; the format-specific
`write_ply` also accepts only a single path.

### Seam-Expansion Adapter

`expand_at_seams(mesh, uvmap) -> std::pair<MeshT, std::vector<Vec<T,2>>>`
lives in `include/educelab/core/utils/MeshUtils.hpp`. `write_ply` with a
`uvmap` calls it internally. The returned flat per-vertex UV array is written
as `s t` properties. This adapter is independently useful for GPU vertex
buffer upload.

---

_Updated 2026-03-24 to reflect interface design review._

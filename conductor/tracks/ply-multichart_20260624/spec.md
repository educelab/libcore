# Specification: Multi-Chart PLY Write Support

**Track ID:** ply-multichart_20260624
**Type:** Feature
**Created:** 2026-06-24
**Status:** Draft

## Summary

Add a multi-chart (multi-texture) `write_ply` overload that mirrors the existing
multi-chart `write_obj`, so meshes textured across several charts can be written
to PLY — not just OBJ — with per-corner chart indices that round-trip on read.

## Context

EduceLab Core provides standard Mesh IO (OBJ and PLY read/write) including UV
maps and texture paths. The `mesh-io_20260323` track shipped PLY texture support
as **single-chart only**: `write_obj` has a
`std::vector<std::filesystem::path>` overload that groups faces by chart and
emits per-chart `usemtl materialN`, but `write_ply` has only a single-path
overload that emits one `comment TextureFile` line.

That asymmetry was a deliberate scope decision recorded in
`mesh-io_20260323/spec.md` ("Multi-texture PLY write — no well-supported standard
outside MeshLab"). This track revisits that decision and closes the gap, adopting
the MeshLab convention as the interchange format. Note that `read_ply` **already**
recovers multiple `comment TextureFile` lines into its `texture_paths`
out-parameter (covered by `PLYTest.ReadCommentTextureFile_HandCraftedPLY`); PLY
can already read multi-chart texture references — it just cannot write them, nor
recover which face belongs to which chart.

## User Story

As an EduceLab developer working with multi-chart textured meshes, I want to
write them to PLY (not only OBJ) so that I can use PLY as my interchange format
end-to-end without losing per-chart texture assignments.

## Acceptance Criteria

- [ ] A `write_ply` overload exists taking
      `const std::vector<std::filesystem::path>& texture_paths`, mirroring the
      multi-chart `write_obj` signature.
- [ ] It `static_assert`s `traits::has_chart<UVMapT>::value` with a message
      consistent with the multi-chart `write_obj` contract.
- [ ] It emits one `comment TextureFile <path>` line per texture, in chart-index
      order, immediately after the `format` line (the layout `read_ply` already
      parses).
- [ ] It emits a per-face `texnumber` scalar property (MeshLab convention)
      giving each face's chart index, so chart membership is recorded in the PLY
      body, not just the header.
- [ ] `read_ply` parses the per-face `texnumber` property and populates chart
      indices on `uvmap` coordinates when `traits::has_chart<UVMapT>`; silently
      ignores it otherwise (consistent with the existing trait-gated behavior).
- [ ] Round-trip test: write a 2-chart mesh, read it back, recover both texture
      paths **and** per-corner chart indices for every face.
- [ ] `read_mesh` / `write_mesh` facade dispatches the new overload for `.ply`.
- [ ] Doxygen `@note`s on `write_ply` / `read_ply` that currently state PLY write
      is single-texture only are updated to describe multi-chart support.
- [ ] Existing single-path `write_ply` overload and single-chart behavior are
      unchanged; all existing MeshIO tests still pass.

## Dependencies

- Builds on completed track `mesh-io_20260323` (Mesh IO).
- Builds on completed track `uv-map_20260323` (`traits::WithChart`,
  `traits::has_chart`).
- Reference implementation: multi-chart `write_obj` overload in
  `include/educelab/core/io/MeshIO_OBJ.hpp`.
- Reference test: `OBJTest.PositionsWithUVs_MultiChart_PerChartUsemtl` in
  `tests/src/TestMeshIO.cpp`.
- Tracks GitHub issue #19.

## Out of Scope

- Binary PLY write (ASCII PLY only, consistent with the existing writer).
- Multiple UV channels per mesh (single `UVMap` per mesh).
- Reconciling chart assignment for faces whose corners disagree on chart index
  (writer keys on the corner-0 chart, matching `write_obj`).
- Texture image loading/saving inside IO functions (callers use `ImageIO`).
- New OBJ behavior — OBJ multi-chart write/read already exists and is unchanged.

## Technical Notes

- **Interchange format — MeshLab convention.** Multi-texture PLY in the wild is
  the MeshLab dialect: per-texture `comment TextureFile <path>` header lines plus
  a per-face `property int texnumber` that names the texture/chart index for each
  face, alongside the existing per-wedge `texcoord` face property. Adopting this
  keeps output readable by MeshLab, Blender, and CloudCompare and is what
  `read_ply` already half-supports (it parses `TextureFile` but not `texnumber`).
- **Chart vs. texnumber.** The UVMap stores a `chart` index per coordinate;
  `texnumber` is its per-face PLY serialization. Writer determines a face's
  `texnumber` from the chart index of its corner-0 UV coordinate, identical to
  how `write_obj` groups faces.
- **Header property ordering matters** for the fixed-layout PLY face element;
  `texnumber` should be emitted in a stable, documented position relative to the
  `vertex_indices` and `texcoord` list properties so the reader can parse it
  deterministically.
- C++17; IO headers remain header-only under `io/`. Follow existing `write_ply`
  tier structure and `split()`-based parsing conventions.

---

_Generated by Conductor. Review and edit as needed._

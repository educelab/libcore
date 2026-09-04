# Specification: PLY Binary IO

**Track ID:** ply-binary-io_20260904
**Type:** Feature
**Created:** 2026-09-04
**Status:** Implemented
**GitHub issue:** [educelab/libcore#25](https://github.com/educelab/libcore/issues/25)

## Summary

Add binary PLY write support, and make `read_ply` honor the endianness declared
in the file header rather than reinterpreting raw bytes as native. These land
together as one contract: a writer that can emit binary must not be able to
produce a file the reader refuses or silently misreads.

## Context

EduceLab Core provides the project's shared mesh IO. `write_ply` today emits
`format ascii 1.0` unconditionally (`MeshIO_PLY.hpp:984`). For a multi-million-face
mesh that is several times the size of the binary equivalent and correspondingly
slower to write — `pgs-decimate` writes its output once per improved search
round and pays that cost repeatedly.

Separately, `read_ply` claims `binary_little_endian` support but does not honor
the declared byte order. `read_ply_binary_prop` (`MeshIO_PLY.hpp:318`) and
`read_ply_prop_from_buf` (`MeshIO_PLY.hpp:388`) reinterpret raw bytes as native
types — native-endian reading labeled little-endian — and `binary_big_endian` is
rejected outright at `MeshIO_PLY.hpp:638`. On the little-endian hosts EduceLab
runs on the mislabeling is invisible; it is still wrong, and it is the half of
the contract the writer would otherwise be able to violate.

## User Story

As an EduceLab developer writing large meshes repeatedly, I want `write_ply` to
emit binary PLY so that output is smaller and faster to write, and I want
`read_ply` to correctly read any binary PLY regardless of the byte order its
header declares.

## Acceptance Criteria

### Write

- [x] A public `educelab::PLYFormat { ASCII, Binary }` exists and is threaded
      through all three `write_ply` tiers, defaulting to `ASCII`.
      *(Task 4.1; `PLYTest.DefaultFormatIsASCII`,
      `PLYTest.ExplicitASCIIMatchesDefault`)*
- [x] `PLYFormat` is kept distinct from `detail::PLYHeader::Format` (three
      values, "what I parsed") so no `detail` type leaks into a public signature.
      *(Task 4.1; separate enum declared outside `namespace detail`)*
- [x] `Binary` writes native byte order and labels the header to match
      (`format binary_little_endian 1.0` on an LE host).
      *(Task 4.6; `PLYTest.BinaryWrite_ByteLevel` asserts the exact line)*
- [x] Scalars are always written as `float32` regardless of the mesh's `T`,
      matching what the ASCII header already declares and what OpenMVS emits.
      The on-disk format does not depend on a template parameter.
      *(Task 4.7; `PLYTest.BinaryWrite_ScalarsAreFloat32RegardlessOfT` asserts
      a `Mesh3d` write is byte-identical to the `Mesh3f` one)*
- [x] The binary writer mirrors the reader's record batching — precomputed
      property offsets, one `write` per vertex — not one `write` per property.
      *(Task 4.7; `detail::write_ply_data_binary`, one `write` per vertex and
      per face record)*
- [x] All three tiers open with `std::ios::binary` unconditionally.
      *(Task 4.8)*
- [x] Existing ASCII output is byte-for-byte unchanged on non-Windows hosts;
      all existing header-grepping tests pass untouched.
      *(Verified by diffing all three tiers' output against `e9635ab`'s writer
      for a mesh with normals, partial colors, partial UVs and mixed face
      arity — identical. No existing test was modified.)*

### Read

- [x] `read_ply` honors the endianness declared in the header, byte-swapping
      when file order differs from host order.
      *(Task 2.4; `PLYTest.BinaryBigEndian_Read`)*
- [x] The `binary_big_endian` rejection at `MeshIO_PLY.hpp:638` is removed and
      `PLYTest.BinaryBigEndian_Throws` is replaced by a positive read test.
      *(Tasks 2.4 and 2.5. The old test was passing for the wrong reason — its
      fixture declares a face but writes no face bytes, so it threw on
      truncation once the rejection was gone.)*
- [x] Swapping enters at the two choke points every binary scalar read passes
      through: `read_ply_binary_prop` (stream) and `read_ply_prop_from_buf`
      (buffer).
      *(Task 2.3; each has one raw-read lambda so the swap cannot be dropped in
      a single case of the width dispatch)*
- [x] The swap flag is a runtime `bool`, not a template parameter.
      *(Task 2.3; `needs_swap`, deliberately with no default so the compiler
      names every call site that omits it — it found all fifteen)*
- [x] The swap applies to the raw fixed-width value **before** the cast to the
      destination type.
      *(Task 2.3; `PLYTest.BinaryBigEndian_SwapPrecedesCast` for the buffer
      path and `..._SwapPrecedesCast_StreamPath` for the stream path)*

### Validation

- [x] `write_ply` throws when a face exceeds the `uchar` list-count limits: 255
      corners for `vertex_indices`, 127 for `texcoord` (which writes `2*N`).
      *(Task 3.2; `PLYTest.FaceOver255Corners_Throws`,
      `FaceOver127CornersWithUVs_Throws`, plus `..._Tier3_Throws` and the two
      non-throwing boundary cases)*
- [x] The message names which limit fired and the offending face index.
      *(Task 3.2; asserted on the message text)*
- [x] `@throws` is documented on the tier-2 and tier-3 `write_ply` overloads and
      the corresponding `write_mesh` dispatchers; tier 1 carries only the 255
      limit.
      *(Task 5.1)*

## Design Decisions

### `PLYFormat` is a new public enum, not `detail::PLYHeader::Format`

`PLYHeader::Format` has three values and answers "what did I parse". The writer
needs two and answers "what should I emit". Reusing it would put a `detail` type
in a public signature and would require the writer to reject one of its own
enum's values.

### Binary writes native order only

A caller who wants a specific byte order is not a caller libcore has. Writing
native and labeling honestly is correct, and the reader now handles either
direction, so libcore-written files remain readable everywhere.

### `float32` on disk regardless of `T`

The ASCII header already declares `property float x`. Making the binary width
follow the mesh's `T` would mean a `Mesh3d` and a `Mesh3f` produce structurally
different files from the same call, and would make the on-disk format a function
of a template parameter. `float32` matches OpenMVS and the existing declaration.

### Runtime `bool` for the swap, not a template parameter

The flag is loop-invariant, so the branch predicts perfectly. Templating on it
doubles the instantiated code for both choke points for no measurable gain.

### Swap before the cast, never after

This is the trap that makes the feature subtle. Big-endian `3F 80 00 00`
`memcpy`'d into a native `float` is 4.6e-41; no reversal of the widened `double`
recovers 1.0. The bytes must be reordered while the value is still its raw
fixed-width type.

### `uchar` list counts stay `uchar`

Widening `vertex_indices` to a `uint32` count would cost 3 bytes per face — some
30MB on a 10M-face mesh — to guard a case that does not occur. Throwing is the
right answer for a mesh that does hit it.

### Endianness helpers are hand-rolled

The library targets `cxx_std_17` (`CMakeLists.txt:69`). `std::endian` is C++20
and `std::byteswap` is C++23, so neither is available; host-order detection and
a width-dispatched swap have to be written against `__BYTE_ORDER__` with a
`_WIN32` fallback.

### The helpers live in `detail`, not a public `utils/Endian.hpp`

*(Task 1.1)* `host_is_little_endian()` and `swap_bytes()` go in
`namespace educelab::detail` in `MeshIO_PLY.hpp`, beside the other PLY-local
helpers, rather than in a new public `utils/Endian.hpp`.

PLY binary IO is the only caller in the library, and byte order is not a problem
any other libcore component has. A public header would have to be added to
`public_hdrs` in `CMakeLists.txt`, installed, Doxygen-documented, and given its
own `TestEndian.cpp` target — permanent public surface and a support obligation
bought for one consumer. `detail` costs nothing and can be promoted the moment a
second caller appears; the reverse move is a breaking change.

This also keeps the track's stated shape: no new files, no new test target,
tests in the existing `tests/src/TestMeshIO.cpp`.

## Testing Strategy

Round-trip tests cannot anchor this work. The sized-alias bug fixed in #24
survived precisely because the suite only ever read what libcore wrote, and a
byte-order mistake shared by reader and writer round-trips just as happily.

- **Reader:** a hand-crafted `binary_big_endian` fixture with bytes reversed by
  the test, exercising the swap path on an LE host. Covers more than one scalar
  width so the dispatch is exercised, not just the 4-byte case.
- **Writer:** byte-level assertion of a small mesh's binary body against a
  hand-derived literal, so the test encodes the format rather than libcore's
  opinion of it.
- **Round-trip:** used only for structural breadth (n-gons, UVs, colors,
  normals, empty mesh) where hand-computing bytes stops paying.

## Dependencies

- **Depends on #24** (sized PLY type aliases on read) — merged as `2eaba49`.
- Depends on existing code: `detail::parse_ply_header`, `detail::PLYHeader`,
  `detail::PLYType`, `detail::ply_type_bytes`, `detail::read_ply_impl`,
  `detail::write_ply_header`, `detail::write_ply_data`, and the three public
  `write_ply` / `read_ply` tiers, all in
  `include/educelab/core/io/MeshIO_PLY.hpp`.
- Tests live in the existing `tests/src/TestMeshIO.cpp`, already registered in
  `tests/CMakeLists.txt`. No new test target.

## Sequencing

Lands **before** [ply-multichart_20260624](../ply-multichart_20260624/index.md)
(#19). Both rewrite `write_ply_header` and `write_ply_data`; multichart is
Pending at 0/15, this is the live need, and this is the smaller and more
additive of the two.

Precedes [mesh-io-options_20260904](../mesh-io-options_20260904/index.md) (#26),
which was filed out of this track's decision to leave `write_mesh` untouched.

## Known Limitation (found during implementation)

MeshLab cannot open **any** PLY libcore writes that combines a `texcoord` list
with a face of more than 3 corners — ASCII or binary, and regardless of this
track. It reports "Face with more than 3 vertices".

Isolated in MeshLab 2025.07 with a matrix varying only arity and `texcoord`:
triangles load with and without texcoord, a quad loads without texcoord, and
only the pair fails — in ASCII and binary identically. So the binary writer
adds no incompatibility of its own.

The limitation is in MeshLab's importer. Its bundled `libio_base.so` carries
vcglib's `import_ply.h` error table, including "Face with no 6 texture
coordinates": per-wedge `texcoord` is hard-coded to 6 floats — 3 corners — so
vcglib's polygonal face path is disabled once texcoords are present. Files that
MeshLab refuses still parse correctly against an independent PLY reader, and
`read_ply` round-trips them.

Verified not to be a regression: the failing ASCII tier-3 file is byte-identical
(md5 `639a03ea…`) to the output of `e9635ab`, the commit this track branched
from. Triangle-only meshes — what the EduceLab pipelines actually write — are
unaffected.

Deferred to [ply-multichart_20260624](../ply-multichart_20260624/index.md)
(#19), which already rewrites the `texcoord` write path and is the right place
to decide between triangulating on write, warning, or documenting.

## Out of Scope

- **A format parameter on `write_mesh`.** A PLY-only value is meaningless for
  half of `write_mesh`'s inputs, and there is no good answer for
  `write_mesh("out.obj", mesh, PLYFormat::Binary)`. Deferred to #26; `write_mesh`
  gains only `@throws` documentation here.
- **Writing non-native byte order.** The writer emits native and labels it.
- **Widening list-count declarations** beyond `uchar`.
- **Binary OBJ**, which is not a format.
- **Multi-chart PLY write** (`texnumber`, multiple `TextureFile` comments) —
  that is #19.
- **Configurable scalar width on write.** `float32` always.

## Technical Notes

- The reader's existing face-record caps (`kMaxFaceVertices` = 256,
  `kMaxFaceListLength` = 1024 in `read_ply_face_binary`) sit above the writer's
  new limits (255 corners, 254 texcoord values), so nothing the writer emits can
  be refused on read. Verify this rather than assume it.
- Opening the ASCII tiers with `std::ios::binary` is a silent behavior change
  for Windows callers, who stop getting CRLF line endings. Nothing regresses on
  read: `read_ply` already trims `\r` (`PLYTest.ReadCommentTextureFile_CRLFLineEndings`
  covers it). Call it out in the PR description.
- `read_ply_face_binary` and the `skip_binary_prop` lambda in `read_ply_impl`
  both call `read_ply_binary_prop`; every call site needs the new flag threaded
  through, not just the vertex path.

---

_Generated by Conductor from educelab/libcore#25. Review and edit as needed._

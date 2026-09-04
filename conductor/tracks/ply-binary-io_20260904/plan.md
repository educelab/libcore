# Implementation Plan: PLY Binary IO

**Track ID:** ply-binary-io_20260904
**Spec:** [spec.md](./spec.md)
**Created:** 2026-09-04
**Status:** [~] In Progress

> Task detail is at planning granularity. Per-task test lists get filled in when
> each phase starts, per the strict tests-first workflow. Design detail lives in
> [spec.md](./spec.md) and [educelab/libcore#25](https://github.com/educelab/libcore/issues/25).

## Overview

Read before write. The reader is where the existing bug is, it is the smaller
change, and a correct reader is the only instrument that can check the writer —
so it lands first and the writer's tests get to lean on it. Validation sits
between them because it is a precondition of the binary writer being safe, not a
polish step.

**Phases 1–2 stand alone**: they fix a real correctness bug (endianness ignored
on read) and are mergeable without any writer work. Phases 3–4 add the feature.

All work is in `include/educelab/core/io/MeshIO_PLY.hpp` and
`tests/src/TestMeshIO.cpp`, with doc-only touches to
`include/educelab/core/io/MeshIO.hpp`. No new files, no new test target.

## Checkpoints

| Phase   | Checkpoint SHA | Date | Status  |
| ------- | -------------- | ---- | ------- |
| Phase 1 | `5861ee1` | 2026-09-04 | verified |
| Phase 2 | `3483f88` | 2026-09-04 | verified |
| Phase 3 |                |      | pending |
| Phase 4 |                |      | pending |
| Phase 5 |                |      | pending |

---

## Phase 1: Byte-order foundation

The library targets `cxx_std_17`, so `std::endian` (C++20) and `std::byteswap`
(C++23) are both unavailable. Host-order detection and a width-dispatched swap
have to be written by hand, and they are worth isolating from the PLY logic that
consumes them.

### Tasks

- [x] **Task 1.1**: Decide where the helpers live — `namespace detail` in
      `MeshIO_PLY.hpp` (assumed: PLY-local, adds no public surface) or a new
      public `utils/Endian.hpp`. Record the decision and rationale in
      `spec.md`.
- [x] **Task 1.2**: Tests for host-order detection and byte swapping across all
      four widths the format uses (1, 2, 4, 8 bytes), including the 1-byte
      no-op and a `double` case.
- [x] **Task 1.3**: Implement host-endianness detection (`__BYTE_ORDER__` where
      the compiler defines it, `_WIN32` fallback) and the width-dispatched
      swap. `<cstring>`/`<cstdint>` only — no new dependency, no new include of
      anything platform-specific.

### Verification

- [x] Helper tests pass
- [x] Build succeeds in Debug and Release
- [x] Full existing suite still green (nothing consumes the helpers yet)

---

## Phase 2: Reader honors declared endianness

Removes the `binary_big_endian` rejection and makes `binary_little_endian` mean
what it says. Swapping enters at the two choke points every binary scalar read
passes through.

### Tasks

- [x] **Task 2.1**: Hand-crafted `binary_big_endian` fixture test — bytes
      reversed by the test itself, positions plus an `int32` index list, read
      back on an LE host. Exercises more than the 4-byte width so the dispatch
      is covered, not just the common case.
- [x] **Task 2.2**: Test that the swap precedes the cast: a BE `float`
      `3F 80 00 00` must read as 1.0, not 4.6e-41. This is the failure mode the
      spec calls out as unrecoverable if the order is inverted, so it gets its
      own test rather than riding along inside a fixture assertion.
- [x] **Task 2.3**: Add a runtime `bool` swap parameter to
      `read_ply_binary_prop` and `read_ply_prop_from_buf`, swapping the raw
      fixed-width value **before** `static_cast<DestT>`. Not a template
      parameter — see spec.
- [x] **Task 2.4**: In `read_ply_impl`, compute the flag from `hdr.format`
      against host order, delete the `BinaryBE` rejection, and thread it to
      every call site — the batched vertex path, `read_ply_face_binary`, and
      the `skip_binary_prop` lambda. Missing one leaves a silently misread
      property.
- [x] **Task 2.5**: Replace `PLYTest.BinaryBigEndian_Throws` with the positive
      read test from Task 2.1, and update the `read_ply` Doxygen that currently
      says "Supports ASCII and binary-little-endian PLY".

### Verification

- [x] BE fixture and pre-cast-swap tests pass
- [x] `BinaryLittleEndian_Read` and `SizedTypeAliases_BinaryLittleEndian_Read`
      still pass unmodified — the LE path must not have moved
- [x] No remaining reference to the BE rejection in code or docs
- [x] Full suite green, Debug and Release

---

## Phase 3: Face list-count validation

A precondition of the binary writer, not polish: `uchar` list counts silently
truncate, so a 300-corner face would write a header the reader cannot make sense
of. ASCII has the same latent problem and gets the same guard.

### Tasks

- [x] **Task 3.1**: Tests for both limits and both boundaries — a 256-corner
      face throws naming `vertex_indices` and the face index; a 128-corner face
      with a UV map throws naming `texcoord`; 255 corners without UVs and 127
      corners with UVs both write cleanly.
- [~] **Task 3.2**: Implement the two limits in the write path. The message
      names which limit fired and the offending face index.
- [ ] **Task 3.3**: Confirm coherence with the reader's existing caps
      (`kMaxFaceVertices` = 256, `kMaxFaceListLength` = 1024 in
      `read_ply_face_binary`) — everything the writer now permits must still be
      readable. Verify, do not assume.

### Verification

- [ ] Limit tests pass, including both non-throwing boundary cases
- [ ] A maximal legal face round-trips through both ASCII and (once Phase 4
      lands) binary
- [ ] Full suite green, Debug and Release

---

## Phase 4: Binary write

The feature itself. `PLYFormat` and the plumbing land first as a no-op signature
change so the tests that follow have something to compile against; the ASCII
path is untouched until Task 4.6.

### Tasks

- [ ] **Task 4.1**: Add public `enum class PLYFormat { ASCII, Binary }` and
      thread it through the three `write_ply` tiers (defaulting to `ASCII`) into
      `write_ply_header` and `write_ply_data`, which ignore it for now.
      Behavior-neutral; kept distinct from `detail::PLYHeader::Format`.
- [ ] **Task 4.2**: Byte-level writer test — a small positions-only mesh
      written as `Binary`, asserted against a hand-derived byte literal
      (header line, then exact vertex and face bytes). The test encodes the
      format, not libcore's opinion of it.
- [ ] **Task 4.3**: Test that scalars are `float32` regardless of `T` — write a
      `Mesh<double, 3>` and assert 4-byte scalars and a `property float x`
      declaration.
- [ ] **Task 4.4**: Test that `ASCII` remains the default — an unqualified
      `write_ply` call still produces `format ascii 1.0`, asserted explicitly
      rather than left to the existing header-grepping tests.
- [ ] **Task 4.5**: Round-trip tests for structural breadth — n-gon faces, UVs,
      colors, normals, empty mesh — where hand-computing bytes stops paying.
- [ ] **Task 4.6**: `write_ply_header` emits the `format` line matching the
      requested format and the host's byte order.
- [ ] **Task 4.7**: Binary path in `write_ply_data` — precomputed property
      offsets, one `write` per vertex record, `float32` scalars, `uchar`
      colors, `uchar`-prefixed `vertex_indices` and `texcoord` lists. Mirrors
      the reader's batching rather than writing per property.
- [ ] **Task 4.8**: Open all three tiers with `std::ios::binary`
      unconditionally. Note in the PR that Windows ASCII callers stop getting
      CRLF; `read_ply` already trims `\r`, so nothing regresses on read.

### Verification

- [ ] Byte-level, `float32`-width, default-ASCII and round-trip tests all pass
- [ ] Existing ASCII tests pass unmodified
- [ ] A binary file written by libcore opens correctly in MeshLab
- [ ] Full suite green, Debug and Release

---

## Phase 5: Documentation

### Tasks

- [ ] **Task 5.1**: `@throws` for the list-count limits on the tier-2 and
      tier-3 `write_ply` overloads and the corresponding `write_mesh`
      dispatchers in `MeshIO.hpp`. Tier 1 carries only the 255 limit.
- [ ] **Task 5.2**: Document `PLYFormat` on the `write_ply` overloads, and note
      that `write_mesh` deliberately gains no format parameter — deferred to
      [#26](https://github.com/educelab/libcore/issues/26) — so the omission
      reads as a decision rather than an oversight.
- [ ] **Task 5.3**: Doxygen builds cleanly with no new warnings.

### Verification

- [ ] Doxygen clean
- [ ] Every acceptance criterion in `spec.md` maps to a passing test or a
      merged doc change

---

## Final Verification

- [ ] All acceptance criteria in [spec.md](./spec.md) met
- [ ] Full test suite passes in Debug and Release
- [ ] Doxygen updated for `PLYFormat`, `@throws`, and BE read support
- [ ] PR notes the Windows CRLF change for ASCII callers
- [ ] Ready for review; #19 unblocked to rebase onto the new
      `write_ply_header` / `write_ply_data` signatures

---

_Generated by Conductor. Tasks will be marked [~] in progress and [x] complete._

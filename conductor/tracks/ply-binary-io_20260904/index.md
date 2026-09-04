# Track: PLY Binary IO

**ID:** ply-binary-io_20260904
**Status:** Pending

## Documents

- Specification — not yet written
- Implementation Plan — not yet written

## Summary

Add binary PLY write support, and make `read_ply` honor the endianness declared
in the file header rather than reinterpreting raw bytes as native. These are one
contract and land together: a writer that can emit binary must not produce files
the reader refuses or silently misreads.

Design decisions are recorded on the GitHub issue. In brief:

- Public `educelab::PLYFormat { ASCII, Binary }` on all three `write_ply` tiers,
  defaulting to `ASCII`. Kept separate from `detail::PLYHeader::Format`.
- `Binary` writes native-endian and labels the header accordingly; scalars are
  always `float32` regardless of the mesh's `T`.
- The writer mirrors the reader's record batching — precomputed offsets, one
  `write` per vertex.
- Read swaps bytes when file order differs from host order, at the two choke
  points every binary scalar passes through. The swap applies to the raw
  fixed-width value *before* the cast to the destination type.
- Faces exceeding the `uchar` list-count limits throw: 255 corners for
  `vertex_indices`, 127 for `texcoord` (which writes `2*N`).

Round-trip tests do not anchor this work. The sized-alias bug in #24 survived
because the suite only ever read what libcore wrote; a byte-order mistake shared
by reader and writer round-trips just as happily. The reader gets a hand-crafted
big-endian fixture, the writer a byte-level assertion against hand-derived bytes.

## Sequencing

Lands before [ply-multichart_20260624](../ply-multichart_20260624/index.md)
(#19) — both rewrite `write_ply_header` and `write_ply_data`, multichart is
Pending at 0/15, and this is the live need.

Depends on #24 (sized PLY type aliases).

## Quick Links

- [Back to Tracks](../../tracks.md)
- [Product Context](../../product.md)
- GitHub issue: educelab/libcore#25

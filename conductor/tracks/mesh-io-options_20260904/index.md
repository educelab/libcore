# Track: write_mesh Options Struct

**ID:** mesh-io-options_20260904
**Status:** Pending

## Documents

- Specification — not yet written
- Implementation Plan — not yet written

## Summary

Let callers control backend-specific writer behavior through `write_mesh`
without dropping to the format-specific entry points and losing extension
dispatch.

Deferred from [ply-binary-io_20260904](../ply-binary-io_20260904/index.md),
where `PLYFormat` lands on `write_ply` only: a PLY-only value is meaningless for
half of `write_mesh`'s inputs, and there is no good answer to what
`write_mesh("out.obj", mesh, PLYFormat::Binary)` should do.

Not scoped or scheduled — registered so the intention is discoverable rather
than living only in a design conversation. Open questions are listed on the
GitHub issue.

## Quick Links

- [Back to Tracks](../../tracks.md)
- [Product Context](../../product.md)
- GitHub issue: educelab/libcore#26

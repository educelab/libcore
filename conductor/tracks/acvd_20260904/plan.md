# Implementation Plan: ACVD Remeshing

**Track ID:** acvd_20260904
**Spec:** [spec.md](./spec.md)
**Created:** 2026-09-04
**Status:** [ ] Not Started

> Task detail is at planning granularity. Per-task test lists get filled in when
> each phase starts, per the strict tests-first workflow. Design detail lives in
> [spec.md](./spec.md) and [educelab/libcore#22](https://github.com/educelab/libcore/issues/22).

## Overview

One clustering engine, built in dependency order, with each layer a separate
verifiable phase. **Phases 0–2 are the useful milestone** — they produce a
working uniform remesher that stands alone; later phases add weight policies
against the same engine. Header-only throughout, matching the rest of the
library.

## Checkpoints

| Phase   | Checkpoint SHA | Date | Status  |
| ------- | -------------- | ---- | ------- |
| Phase 0 |                |      | pending |
| Phase 1 |                |      | pending |
| Phase 2 |                |      | pending |
| Phase 3 |                |      | pending |
| Phase 4 |                |      | pending |
| Phase 5 |                |      | pending |
| Phase 6 |                |      | pending |

---

## Phase 0: Design decisions

Two decisions shape the rest of the code and are expensive to revisit. Settle
both against the paper PDFs before the minimizer is written.

### Tasks

- [ ] **Task 0.1**: Decide the clustered item domain — mesh vertices (assumed
      throughout the plan: cluster count equals output vertex count, triangles
      fall out of the dual directly) or mesh faces. Record the decision and
      rationale in `spec.md`.
- [ ] **Task 0.2**: Decide open-mesh boundary treatment. A separate 1D
      clustering along each boundary loop is the presumed approach. This matters
      more for EduceLab meshes than for the papers' test cases — most are open
      patches, where a shrinking or ragged boundary is disqualifying.
- [ ] **Task 0.3**: Add the three reference papers to `docs/citations.bib`
      (already wired into Doxygen via `CITE_BIB_FILES`, currently empty).

### Verification

- [ ] Both decisions recorded in `spec.md` with rationale
- [ ] Doxygen builds with the new bib entries resolving

---

## Phase 1: Mesh topology

Read-only CSR adjacency. ACVD never mutates topology, so no half-edge structure
and no incremental maintenance.

### Tasks

- [ ] **Task 1.1**: Tests for `types/MeshTopology.hpp` in
      `tests/src/TestMeshTopology.cpp`
- [ ] **Task 1.2**: Implement CSR vertex&rarr;vertex, vertex&rarr;face and
      face&rarr;face adjacency; edge table keyed on sorted vertex pairs;
      boundary-loop extraction; crease flags by dihedral angle; a small
      index-view type (C++17 has no `std::span`)
- [ ] **Task 1.3**: Register the test in `tests/CMakeLists.txt` and the header
      in `public_hdrs` and the `README.md` header-only list

### Verification

- [ ] Built in a single O(V+F) pass; no allocation per adjacency query
- [ ] `ctest` passes with no regressions; Debug and Release both build

---

## Phase 2: Uniform ACVD (2004)

### Tasks

- [ ] **Task 2.1**: Tests for the energy bookkeeping — incremental accumulators
      match a from-scratch recompute; the O(1) move test matches full energy
      recomputation
- [ ] **Task 2.2**: Item weights (lumped area) and `utils/MeshQuality.hpp`
      (areas, angle bounds, edge-length statistics)
- [ ] **Task 2.3**: Density-weighted seeding and multi-source BFS growth, with
      every connected component guaranteed at least one seed
- [ ] **Task 2.4**: Energy minimizer — boundary-item queue, the O(1)
      reassignment test, `double` accumulators, mean-centered positions, guards
      against emptying or disconnecting a cluster
- [ ] **Task 2.5**: Cluster repair — reseed empty clusters by splitting the
      highest-energy cluster, split disconnected clusters, re-minimize
- [ ] **Task 2.6**: Dual construction, centroid placement, trait-aware
      attribute transfer via `if constexpr`, and the result/statistics report
- [ ] **Task 2.7**: End-to-end tests — identity case, uniformity on a grid patch
      and subdivided icosphere, output topology validity, determinism,
      degenerate inputs

### Verification

- [ ] Output vertex count is exactly as requested; output is manifold and
      consistently oriented
- [ ] Energy non-increasing across sweeps; converges within `maxSweeps`
- [ ] Deterministic for a fixed seed across Debug and Release
- [ ] Test runtime stays within the CI budget on all five OS images

---

## Phase 3: Adaptive weights and quadric placement (2008, scalar)

### Tasks

- [ ] **Task 3.1**: SPD 3&times;3 solve (Cholesky) added to
      `utils/LinearAlgebra.hpp`, with tests
- [ ] **Task 3.2**: `utils/Curvature.hpp` — per-item curvature estimate
- [ ] **Task 3.3**: Gradation weights and the `WeightFn` overload;
      `density_for_edge_length`
- [ ] **Task 3.4**: Quadric (QEM) site placement regularized toward the
      centroid; boundary line quadrics; crease-corner pinning

### Verification

- [ ] Gradation concentrates vertices in high-curvature regions, measured
- [ ] Quadric placement preserves a known sharp edge better than centroid
      placement
- [ ] Open-mesh boundary vertices stay on the input boundary curve within
      tolerance
- [ ] A two-region density field achieves the predicted mean edge-length ratio,
      validating the `ρ = h⁻⁴` exponent

---

## Phase 4: Anisotropic metrics (2008, full)

Highest risk, least load-bearing for EduceLab meshes. Defer until something
needs anisotropy.

### Tasks

- [ ] **Task 4.1**: Symmetric 3&times;3 eigensolve (Jacobi) in
      `utils/LinearAlgebra.hpp`, with tests
- [ ] **Task 4.2**: Curvature tensor estimation and metric tensor construction
- [ ] **Task 4.3**: Tensor accumulators and the metric energy path, with the
      scalar path kept as a specialization

### Verification

- [ ] `M = ρI` reproduces the scalar path's output exactly
- [ ] Anisotropic metrics produce measurably elongated elements in the
      prescribed direction

---

## Phase 5: Approach-guided density fields (2011)

### Tasks

- [ ] **Task 5.1**: ROI distance helpers (point, segment, polyline)
- [ ] **Task 5.2**: Size&harr;density calibration helper and worked example
- [ ] **Task 5.3**: Element quality report suitable for FE consumers

### Verification

- [ ] A corridor-proximity density field produces the prescribed element size
      gradient, measured

---

## Phase 6: Optional extras

Not required by any acceptance criterion; pull forward only on demand.

### Tasks

- [ ] **Task 6.1**: Subdivision pre-pass for inputs too coarse for the
      requested cluster count
- [ ] **Task 6.2**: Spatial index, enabling surface-projected site placement
- [ ] **Task 6.3**: UV/attribute resampling onto the remeshed output

### Verification

- [ ] Each addition is independently tested and does not alter default behavior

---

## Final Verification

- [ ] All acceptance criteria in `spec.md` and issue #22 met
- [ ] All tests passing (`ctest`), Debug and Release
- [ ] Doxygen builds cleanly for the new public API
- [ ] New headers registered in `CMakeLists.txt` and `README.md`
- [ ] Clean-room constraint upheld — no reference implementation consulted
- [ ] Ready for PR review

---

_Generated by Conductor. Tasks will be marked [~] in progress and [x] complete._

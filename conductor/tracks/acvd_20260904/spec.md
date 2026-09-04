# Specification: ACVD Remeshing

**Track ID:** acvd_20260904
**Type:** Feature
**Created:** 2026-09-04
**Status:** Draft
**GitHub issue:** [educelab/libcore#22](https://github.com/educelab/libcore/issues/22)
**Companion issue:** educelab/OpenABF#62 (adapter/wrapper)

## Summary

Implement ACVD (Approximated Centroidal Voronoi Diagrams) as a mesh
remeshing/coarsening utility in libcore: one clustering engine with layered
weight policies covering uniform coarsening, curvature-adaptive gradation,
anisotropic elements, and caller-supplied spatial density fields.

Design detail below is complete as of 2026-09-04. Genuinely unresolved items are
collected in [Open Questions](#open-questions); everything else is settled.

## Context

EduceLab Core provides common types and utilities for EduceLab C++ projects with
minimal external dependencies. Mesh coarsening and conditioning are important
preprocessing steps for parameterization and downstream analysis, but no such
utility exists in the library today.

The three reference papers describe **one engine, not three algorithms**. What
differs between them is the weight attached to each mesh element and where the
output vertex lands:

| Layer | Contribution |
|-------|--------------|
| Valette & Chassery 2004 | The engine: cluster elements into *n* connected regions minimizing a weighted compactness energy; build the output as the dual. Weight = element area. |
| Valette, Chassery & Prost 2008 | Generalizes the scalar weight to a per-element SPD metric tensor: curvature-adaptive gradation (scalar density), anisotropic elements (full tensor), quadric vertex placement, feature/boundary constraints. |
| Audette et al. 2011 | An application of the 2008 machinery with density from a spatial field (proximity to a surgical approach corridor) rather than from curvature. Collapses to a caller-supplied callable. |

### Why libcore and not OpenABF

- **ACVD needs no sparse linear algebra.** Dense 3&times;3 is the largest system
  it ever solves; the sole exception is a self-contained symmetric eigensolve
  (~80 lines, Jacobi) behind anisotropic metrics. libcore can host all three
  papers **without gaining a single dependency**. OpenABF requires Eigen, so
  hosting the engine there would make Eigen the price of admission for any
  consumer that wants remeshing and nothing else.
- **`Mesh` is a sufficient substrate.** ACVD never mutates topology — it reads
  adjacency, writes one integer label per item, and emits a brand-new mesh. No
  edge collapse, no splicing, no incremental connectivity update. A half-edge
  structure is not required; four flat CSR arrays built in a single O(V+F) pass
  are. OpenABF's `HalfEdgeMesh` is in fact the further of the two from what the
  inner loop wants (`shared_ptr` elements, `std::multimap` edge store,
  `outgoing_edges()` returning a `vector<EdgePtr>` by value, `wheel()` throwing
  on boundary vertices).
- **Dependency direction** stays OpenABF &rarr; libcore (heavy &rarr; light).

## User Story

As an EduceLab developer, I want to coarsen or remesh a `Mesh` to a target
vertex count — uniformly, or with element size driven by curvature or a spatial
field — so that I can condition meshes for parameterization and analysis without
adding dependencies to my project.

## Algorithm

### Energy

Let the clustered **items** be the mesh vertices, each with position `x_s` and
positive weight `ρ_s` (its lumped area: one third of the summed area of incident
triangles). A labelling assigns every item to one of *n* clusters. The energy is
the weighted moment of inertia of each cluster about its own centroid:

```
E = Σ_i Σ_{s ∈ C_i} ρ_s ‖x_s − γ_i‖²      γ_i = (Σ ρ_s x_s) / (Σ ρ_s)
```

Clusters are constrained to be connected sets of mesh items, and distances are
measured in R³ rather than along the surface. Those two approximations are what
the *A* in ACVD stands for, and what makes the algorithm near-linear.

### The reassignment test

Removing item `(ρ, p)` from cluster *a* lowers that cluster's energy by exactly
`m_a·ρ/(m_a − ρ)·‖γ_a − p‖²`; adding it to *b* raises *b*'s by the same
expression with the sign of `ρ` flipped in the denominator. Both `ρ` factors
cancel:

```
move s from a to b  ⟺  m_a‖γ_a − p‖²/(m_a − ρ_s)  >  m_b‖γ_b − p‖²/(m_b + ρ_s)
```

A mass-corrected nearest-centroid test: eight flops and two squared distances,
no global constant, no reference to the origin. Verified against brute-force
energy recomputation over 400 random reassignments — maximum relative error
1.8e-13 in double precision.

Guard with `m_a − ρ_s > 0` and a strict improvement margin: a cluster must never
be emptied (that silently drops an output vertex), and an epsilon-free
comparison cycles between equal-energy states forever.

### Numerical constraint: do not use the subtractive energy form

Expanding the square gives `E = Q − Σ_i ‖c_i‖²/m_i` with `m_i = Σρ_s`,
`c_i = Σρ_s x_s`, `Q = Σ_s ρ_s‖x_s‖²`. This is algebraically correct and
tempting (four accumulated scalars per cluster), but `Q` and `Σ‖c‖²/m` are both
origin-dependent and nearly equal, so their difference is destroyed by
cancellation.

Measured in float32 on 5,000 items offset to (8000, 4000, 12000) with 200
clusters:

| Form | Result |
|------|--------|
| True energy (f64, centroid form) | 57,442.31 |
| `Q − Σ‖c‖²/m` in f32 | **−262,144.00** |
| Centroid form in f32 | 57,442.39 |

The subtractive form returns a *negative* energy — a minimizer driven by its
sign will wander. Therefore: mean-center item positions once up front,
accumulate `m` and `c` in `double` regardless of the mesh's `T`, and never
evaluate the energy by subtraction.

### Metric generalization (2008)

Replace the scalar weight with a symmetric positive-definite tensor `M_s` and
the squared distance with the quadratic form `(x − γ)ᵀM(x − γ)`:

```
A_i = Σ M_s      b_i = Σ M_s x_s      k_i = Σ x_sᵀ M_s x_s
γ_i = A_i⁻¹ b_i                       E_i = k_i − b_iᵀ A_i⁻¹ b_i
```

Ten doubles per cluster instead of four. `M = ρI` recovers the scalar case
exactly, so one implementation covers both — but the scalar path gets its own
specialization, since the general path costs two 3&times;3 SPD solves per
candidate move instead of two squared distances. `A` is a sum of SPD tensors, so
Cholesky is safe.

### Density and target edge length

Asymptotic optimal site density for a CVD in *d* dimensions goes as
`ρ^(d/(d+2))`; on a surface (*d* = 2) that is `ρ^(1/2)`, and site density is
`1/h²`, giving:

```
h ∝ ρ^(−1/4)   ⟺   ρ = h⁻⁴
```

Exposed as `density_for_edge_length(h)`. This is an asymptotic relation, so it is
treated as a calibration and verified numerically — a two-region size field is
the natural test and the place an off-by-one in the exponent surfaces.

### Output construction

The output is the dual of the clustering: one vertex per cluster, and one
triangle for every input triangle whose three corners carry three distinct
labels. Winding carries over from the input triangle, so orientation is free.
Every property wanted from the result — manifoldness, valence, no holes — is a
statement about the cluster adjacency graph, which is why cluster connectivity
must be enforced during minimization rather than checked afterward.

## Pipeline

1. **Topology & preconditions** — build CSR adjacency; filter zero-area faces;
   assert `clusters <= item count`; warn below ~8 items per cluster (below that
   the dual degenerates and the input needs subdividing first).
2. **Item weights** — lumped area, times `curvature^gradation` for adaptive or a
   caller-supplied spatial density for approach-guided. This single vector is the
   entire difference between the 2004 and 2011 papers.
3. **Per-item metric tensors** — anisotropic path only.
   `M = R diag(f(κ₁), f(κ₂), f(n)) Rᵀ`.
4. **Seeding** — *n* density-weighted seeds, then simultaneous multi-source BFS
   so every initial cluster is connected by construction. Every connected
   component of the input must get at least one seed or it vanishes from the
   output.
5. **Energy minimization** — sweep the boundary-item queue applying the O(1)
   test; on an accepted move, re-push the item's neighbours. Reject moves that
   would empty or disconnect a cluster (cheap version of the second check: a
   one-ring connectivity test on the departing cluster).
6. **Cluster repair** — reseed empty clusters by splitting the highest-energy
   cluster; split clusters that have become disconnected; re-minimize. This is
   the stage most likely to be underestimated; it is where a plausible-looking
   implementation stops producing manifold output.
7. **Site placement** — centroid, surface-projected centroid, or quadric
   minimization over the cluster's face planes regularized toward the centroid.
   Boundary clusters take line quadrics from their boundary edges; crease corners
   get pinned.
8. **Dual construction & validity** — emit one triangle per 3-label input face,
   then repair what the dual cannot express: clusters with fewer than three
   neighbours, duplicated triangles, edges with more than two incident faces,
   open-mesh boundaries.
9. **Attribute transfer & report** — average normals and colors per cluster
   behind `if constexpr` on the existing `traits::has_normal` /
   `traits::has_color`. Return the label vector, final energy, and quality
   statistics.

## Proposed API

```cpp
// include/educelab/core/utils/Remeshing.hpp
enum class ClusterDomain { Vertices, Faces };
enum class SitePlacement { Centroid, Projected, Quadric, QuadricRegularized };

template <typename T = float>
struct RemeshOptions {
    std::size_t   clusters{0};        // == desired output vertex count
    ClusterDomain domain{ClusterDomain::Vertices};
    SitePlacement placement{SitePlacement::QuadricRegularized};
    T             gradation{0};       // 0 = uniform (2004); >0 = curvature-adaptive (2008)
    T             featureAngle{to_radians<T>(60)};
    bool          preserveBoundary{true};
    std::size_t   maxSweeps{200};
    T             tolerance{1e-6};
    std::uint64_t seed{0};            // explicit: educelab::random() is not seedable
    Signal<std::size_t, double>* progress{nullptr};  // (sweep, energy)
};

template <typename T, std::size_t Dims, typename VTraits>
struct RemeshResult {
    Mesh<T, Dims, VTraits>   mesh;    // the remeshed output
    std::vector<std::size_t> labels;  // cluster id per input item
    double                   energy{};
    std::size_t              sweeps{};
    std::size_t              repairs{};
};

// 2004 uniform, and 2008 curvature-adaptive via opts.gradation
template <typename T, std::size_t Dims, typename VTraits>
[[nodiscard]] auto acvd_remesh(const Mesh<T, Dims, VTraits>& mesh,
                               const RemeshOptions<T>& opts)
    -> RemeshResult<T, Dims, VTraits>;

// 2008 generic / 2011 approach-guided: caller supplies the field.
//   WeightFn: T(const Vec<T,Dims>&)          -> scalar density
//   WeightFn: Mat<3,3,T>(const Vec<T,Dims>&) -> SPD metric tensor
template <typename T, std::size_t Dims, typename VTraits, typename WeightFn>
[[nodiscard]] auto acvd_remesh(const Mesh<T, Dims, VTraits>&,
                               const RemeshOptions<T>&, WeightFn&&)
    -> RemeshResult<T, Dims, VTraits>;

// Asymptotic CVD size relation: rho = h^-4
template <typename T>
[[nodiscard]] constexpr auto density_for_edge_length(T h) -> T;
```

The 2011 layer is then entirely on the caller's side of the boundary:

```cpp
// Fine elements within 10 mm of the approach axis, coarsening to 4 mm beyond it.
auto density = [&](const Vec3f& p) {
    const auto d = distance_to_segment(p, entry, target);
    const auto h = std::clamp(0.5f + 0.35f * (d - 10.f), 0.5f, 4.f);
    return density_for_edge_length(h);
};
auto result = acvd_remesh(brain, opts, density);
```

Two smaller headers carry the rest: `types/MeshTopology.hpp` for CSR adjacency
and edge queries, and `utils/MeshQuality.hpp` for areas, angle bounds and
edge-length statistics — both independently useful and testable before any
clustering exists.

## Acceptance Criteria

### Phase 1 — topology

- [ ] `MeshTopology` provides vertex&rarr;vertex, vertex&rarr;face and
      face&rarr;face adjacency in CSR form, an edge table with incident faces,
      boundary-loop extraction, and crease flags by dihedral angle
- [ ] Built in a single O(V+F) pass; no allocation per adjacency query
- [ ] Tests in `tests/src/TestMeshTopology.cpp`, registered in
      `tests/CMakeLists.txt`

### Phase 2 — uniform ACVD

- [ ] `acvd_remesh` produces exactly `opts.clusters` output vertices for valid
      inputs
- [ ] Incremental cluster accumulators match a from-scratch recompute after N
      random reassignments
- [ ] The O(1) move test matches a full energy recomputation (regression test for
      the formula above)
- [ ] Energy is non-increasing across sweeps; convergence within `maxSweeps` on
      all fixtures
- [ ] Identity case: `clusters == item count` reproduces the input mesh
- [ ] Uniformity: on a regular grid patch and a subdivided icosphere,
      edge-length coefficient of variation is below a documented bound; sphere
      output vertices lie on the sphere within tolerance
- [ ] Topology: every output edge has ≤2 incident faces, orientation is
      consistent, Euler characteristic is preserved for closed genus-0 input,
      boundary loop count is preserved for an open patch
- [ ] Determinism: same seed &rarr; identical output, in-process and across
      Debug/Release
- [ ] Degenerate inputs have defined, tested behavior: empty mesh,
      `clusters == 0`, `clusters > item count`, zero-area faces, duplicate
      vertices, disconnected components, non-manifold edges
- [ ] Per-vertex normals and colors transfer when the vertex traits carry them,
      and are absent when they don't

### Phases 3–5 — adaptive, anisotropic, approach-guided

- [ ] Gradation > 0 concentrates vertices in high-curvature regions (measured,
      not eyeballed)
- [ ] Quadric placement preserves a crease better than centroid placement on a
      fixture with a known sharp edge
- [ ] Open-mesh boundaries do not shrink; boundary vertex positions stay on the
      input boundary curve within tolerance
- [ ] A two-region density field achieves the predicted mean edge-length ratio,
      validating the `ρ = h⁻⁴` exponent
- [ ] `M = ρI` reproduces the scalar path's output exactly
- [ ] Anisotropic metrics produce measurably elongated elements in the prescribed
      direction

### Docs & build

- [ ] Three papers added to `docs/citations.bib`, referenced via `@cite` from the
      new headers
- [ ] New headers listed in `public_hdrs` in the root `CMakeLists.txt` and in the
      header-only list in `README.md`
- [ ] Doxygen builds cleanly for the new public API

## Testing Strategy

Golden-file comparison is the wrong instrument: output depends on seeded RNG and
float accumulation order across five CI images, and a remesher has no canonical
answer. Test invariants and statistics instead, on meshes generated in the test
itself — CI runs Release on five OS images, so every case needs to finish in
about a second.

Priority order: accumulator agreement, move test against brute force,
monotonicity, identity case, uniformity statistics, topology validity, adaptive
calibration, determinism, degenerate inputs.

## Dependencies

Depends on existing code:

- `types/Mesh.hpp` — clustering input and output construction
- `types/Vec.hpp`, `utils/Math.hpp` — vector math
- `types/Mat.hpp`, `utils/LinearAlgebra.hpp` — **needs additions**: SPD
  3&times;3 solve (Cholesky) for quadrics and the metric path; symmetric
  3&times;3 eigensolve (Jacobi) for the anisotropic layer only. Today only
  `determinant` and `solve_cramer` exist — no inverse, no Cholesky
- `types/Signals.hpp` — optional per-sweep progress reporting

No new external dependencies.

### Hazards in existing code

- `normalize()` has no zero guard, so `Mesh::face_normal` returns NaN for a
  zero-area triangle, and those NaNs propagate into quadrics and curvature
  tensors where they are hard to trace back. Filter degenerate faces in stage 1
  and treat their weight as zero
- `interior_angle()` does not clamp its `acos` argument — dot products can land
  marginally outside [-1, 1] in float. `vertex_normal` is built on it
- `norm()` accumulates in the element type; fine for one vector, wrong for
  cluster accumulators
- `Mesh::insert_vertex` clears the whole face-normal cache (O(F) per insertion),
  so interleaving vertex and face insertion during output construction is
  quadratic. Insert all vertices first, then all faces
- `Mesh::vertex_faces` returns a `vector<vector>` — ~48 B/vertex of overhead and
  poor locality at scale, hence the CSR structure in phase 1
- `Mesh` has no element removal, and mutating `face()` does not invalidate
  caches. Neither blocks this work (ACVD builds a fresh output mesh), but repair
  passes must not try to edit in place
- `educelab::random` is **not seedable** (static `mt19937` from
  `random_device`), so the engine owns its own generator behind an explicit seed
- `docs/citations.bib` is wired into Doxygen (`CITE_BIB_FILES`) and empty

## Out of Scope

- **Deviation-metric-driven iterative coarsening** (coarsen until surface
  deviation exceeds a tolerance). Scoped and deliberately dropped: it needs a
  spatial index plus two-sided sampled Hausdorff measurement, and the ACVD energy
  is a compactness metric, not a deviation metric, so error control would have to
  be layered on rather than read off
- **QEM edge-collapse decimation.** Better matched to "fewest vertices under a
  hard error bound", but a different algorithm — and the one case where a mutable
  half-edge structure would genuinely earn its keep
- **Parallelism.** The boundary sweep is inherently sequential; single-threaded
  is the target
- **The OpenABF adapter**, tracked in educelab/OpenABF#62
- Subdivision pre-pass, spatial index, and UV/attribute resampling are phase 6
  extras, not required by any acceptance criterion

## Open Questions

Unresolved; to settle against the paper PDFs. Items 1 and 2 are the decisions the
rest of the code shapes itself around — cheap to settle now, expensive to
revisit — and are gated as phase 0 of the plan.

1. **Vertices or faces as the clustered items?** This spec assumes vertex
   clustering, which makes the requested cluster count equal the output vertex
   count and yields triangles directly by duality. Face clustering is the
   alternative and changes the dual rule and the boundary handling
2. **Boundary treatment for open meshes.** A separate 1D clustering along each
   boundary loop (so the output boundary is a polyline through boundary clusters)
   is the natural approach. This matters more for EduceLab meshes than for the
   papers' test cases — most are open patches, where a shrinking or ragged
   boundary is disqualifying
3. **The exact gradation formula** — the shape of the curvature-to-density map
   and the clamping that keeps it stable in flat regions
4. **Which curvature estimator** the 2008 paper uses; standard choices differ
   noticeably on noisy meshes
5. **How much topology repair is enough?** The papers acknowledge that clusters
   can become disconnected and that the dual can be invalid, but a complete
   repair strategy is likely sketched in the paper and load-bearing in practice.
   Budget for iteration
6. **UV maps.** Remeshing invalidates a `UVMap` outright. Dropping it is honest;
   resampling requires nearest-face queries and a spatial index. Decide the
   contract explicitly rather than leaving callers to discover it

## Effort Estimate

| Phase | Scope | Estimate |
|-------|-------|----------|
| 0 | Design decisions, citations | ~0.5 d |
| 1 | MeshTopology | ~1 d |
| 2 | Uniform ACVD (2004) | 4–5 d |
| 3 | Adaptive + quadric placement (2008 scalar) | 3–4 d |
| 4 | Anisotropic (2008 full) | 3–4 d |
| 5 | Approach-guided (2011) | 1–2 d |
| 6 | Optional extras | 2–3 d |

Phases 0–2 are the useful milestone: roughly a week for a working uniform
remesher that stands alone. Phase 4 is the riskiest and least load-bearing for
EduceLab meshes; phase 5 is nearly free once phase 3 lands, which is the
strongest argument for treating the three papers as one engine.

## Clean-Room Constraint

A reference implementation of this algorithm exists online. This must be a **pure
implementation from the reference papers — that reference library MUST NOT be
examined.** Everything in this specification was derived from the papers'
described methods and verified numerically, with no reference implementation
consulted. Cite the papers via `@cite` from the new headers as the provenance
record.

## References

- Valette, S., and J.-M. Chassery. "Approximated centroidal Voronoi diagrams for
  uniform polygonal mesh coarsening." *Computer Graphics Forum* 23, no. 3 (2004):
  381–389.
  [PDF](https://hal.science/file/index/docid/534535/filename/valette.pdf)
- Valette, S., J.-M. Chassery, and R. Prost. "Generic remeshing of 3D triangular
  meshes with metric-dependent discrete Voronoi diagrams." *IEEE TVCG* 14, no. 2
  (2008): 369–381. doi:10.1109/TVCG.2007.70430
  [PDF](https://hal.science/hal-00537025/document)
- Audette, M., D. Rivière, M. Ewend, A. Enquobahrie, and S. Valette.
  "Approach-guided controlled resolution brain meshing for FE-based interactive
  neurosurgery simulation." *MeshMed @ MICCAI 2011*: 176–186.
- GitHub issue: educelab/libcore#22
- Companion issue: educelab/OpenABF#62

---

_Generated by Conductor. Review and edit as needed._

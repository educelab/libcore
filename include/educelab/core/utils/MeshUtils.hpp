#pragma once

/** @file */

#include <limits>
#include <map>
#include <utility>
#include <vector>

#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/Vec.hpp"

namespace educelab
{

/**
 * @brief Expand a mesh at UV seams to produce a per-vertex UV array
 *
 * Walks every face corner. When a vertex already carries a different UV
 * coordinate than the one requested by the current corner, the vertex is
 * duplicated (position and all traits copied). The result is a mesh in which
 * each vertex maps to exactly one UV coordinate, enabling formats that store
 * UV data per-vertex (e.g. PLY).
 *
 * Vertices with no UV mapping in @p uvmap receive a zero UV vector and are
 * deduplicated across corners that share the same (vertex, no-UV) state.
 *
 * @return A pair of:
 *   - The expanded @c Mesh (may have more vertices than the original if seams
 *     were present)
 *   - A flat @c std::vector of @c Vec<UVT,2> indexed by new vertex index,
 *     suitable for direct per-vertex UV output
 *
 * @tparam T         Mesh numeric type
 * @tparam Dims      Mesh dimensionality
 * @tparam VTraits   Vertex traits type
 * @tparam UVMapT    UV map type (any @ref UVMap instantiation)
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
[[nodiscard]] auto expand_at_seams(
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT& uvmap)
    -> std::pair<
        Mesh<T, Dims, VTraits>,
        std::vector<Vec<typename UVMapT::Coordinate::value_type, 2>>>
{
    using MeshT = Mesh<T, Dims, VTraits>;
    using UVT = typename UVMapT::Coordinate::value_type;
    using UVVec = Vec<UVT, 2>;

    MeshT expanded;
    std::vector<UVVec> flat_uvs;

    // (original_vertex_idx, uv_pool_idx) → new_vertex_idx
    // SIZE_MAX is used as the sentinel "no UV mapping"
    constexpr auto kNoUV = std::numeric_limits<std::size_t>::max();
    std::map<std::pair<std::size_t, std::size_t>, std::size_t> vertex_map;

    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        const auto& face = mesh.face(fi);
        typename MeshT::Face new_face;
        new_face.reserve(face.size());

        for (std::size_t ci = 0; ci < face.size(); ++ci) {
            const auto vi = face[ci];
            const auto uvi =
                uvmap.has(fi, ci) ? uvmap.get(fi, ci) : kNoUV;

            const auto key = std::make_pair(vi, uvi);
            const auto [it, inserted] = vertex_map.emplace(key, expanded.num_vertices());

            if (inserted) {
                // First time this (vertex, uv) pair is seen — create new vertex
                (void)expanded.insert_vertex(mesh.vertex(vi));
                if (uvi != kNoUV) {
                    const auto& c = uvmap.at(uvi);
                    flat_uvs.push_back(UVVec{c[0], c[1]});
                } else {
                    flat_uvs.push_back(UVVec{});
                }
            }
            new_face.push_back(it->second);
        }

        (void)expanded.insert_face(new_face);
    }

    return {std::move(expanded), std::move(flat_uvs)};
}

}  // namespace educelab

#pragma once

/** @file */

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "educelab/core/types/Color.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"
#include "educelab/core/utils/String.hpp"

namespace educelab
{

namespace detail
{

/** @brief Convert a Color to normalised [0,1] float RGB */
inline auto color_to_rgb(const Color& c) -> std::array<float, 3>
{
    switch (c.type()) {
        case Color::Type::F32C3: {
            const auto v = c.value<Color::F32C3>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::F32C4: {
            const auto v = c.value<Color::F32C4>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::U8C3: {
            const auto v = c.value<Color::U8C3>();
            return {v[0] / 255.f, v[1] / 255.f, v[2] / 255.f};
        }
        case Color::Type::U8C4: {
            const auto v = c.value<Color::U8C4>();
            return {v[0] / 255.f, v[1] / 255.f, v[2] / 255.f};
        }
        case Color::Type::U16C3: {
            const auto v = c.value<Color::U16C3>();
            return {v[0] / 65535.f, v[1] / 65535.f, v[2] / 65535.f};
        }
        case Color::Type::U16C4: {
            const auto v = c.value<Color::U16C4>();
            return {v[0] / 65535.f, v[1] / 65535.f, v[2] / 65535.f};
        }
        case Color::Type::F32C1: {
            const auto v = c.value<Color::F32C1>();
            return {v, v, v};
        }
        case Color::Type::U8C1: {
            const float v = c.value<Color::U8C1>() / 255.f;
            return {v, v, v};
        }
        case Color::Type::U16C1: {
            const float v = c.value<Color::U16C1>() / 65535.f;
            return {v, v, v};
        }
        default:
            return {0.f, 0.f, 0.f};
    }
}

/**
 * @brief Parse a single OBJ face vertex reference (@c "v/vt/vn", @c "v//vn",
 *        @c "v/vt", or @c "v")
 *
 * @return {v_idx, vt_idx, vn_idx} — all 0-based; vt/vn are nullopt when absent
 */
inline auto parse_face_ref(std::string_view token)
    -> std::tuple<
        std::size_t,
        std::optional<std::size_t>,
        std::optional<std::size_t>>
{
    // Find first '/'
    const auto p1 = token.find('/');
    if (p1 == std::string_view::npos) {
        if (!token.empty() && token[0] == '-') {
            throw std::runtime_error(
                "read_obj: negative (relative) face indices are not supported");
        }
        const auto raw = to_numeric<std::size_t>(token);
        if (raw == 0) {
            throw std::runtime_error(
                "read_obj: face index 0 is invalid (OBJ indices are 1-based)");
        }
        return {raw - 1, std::nullopt, std::nullopt};
    }

    const auto v_sv = token.substr(0, p1);
    if (!v_sv.empty() && v_sv[0] == '-') {
        throw std::runtime_error(
            "read_obj: negative (relative) face indices are not supported");
    }
    const auto raw_v = to_numeric<std::size_t>(v_sv);
    if (raw_v == 0) {
        throw std::runtime_error(
            "read_obj: face index 0 is invalid (OBJ indices are 1-based)");
    }
    const auto v = raw_v - 1;
    const auto rest = token.substr(p1 + 1);

    // Find second '/'
    const auto p2 = rest.find('/');
    if (p2 == std::string_view::npos) {
        // "v/vt"
        if (rest.empty()) {
            return {v, std::nullopt, std::nullopt};
        }
        if (!rest.empty() && rest[0] == '-') {
            throw std::runtime_error(
                "read_obj: negative (relative) face indices are not supported");
        }
        const auto raw_vt = to_numeric<std::size_t>(rest);
        if (raw_vt == 0) {
            throw std::runtime_error(
                "read_obj: face index 0 is invalid (OBJ indices are 1-based)");
        }
        return {v, raw_vt - 1, std::nullopt};
    }

    // "v/vt/vn" or "v//vn"
    const auto vt_sv = rest.substr(0, p2);
    const auto vn_sv = rest.substr(p2 + 1);

    std::optional<std::size_t> vt_idx;
    if (!vt_sv.empty()) {
        if (vt_sv[0] == '-') {
            throw std::runtime_error(
                "read_obj: negative (relative) face indices are not supported");
        }
        const auto raw_vt = to_numeric<std::size_t>(vt_sv);
        if (raw_vt == 0) {
            throw std::runtime_error(
                "read_obj: face index 0 is invalid (OBJ indices are 1-based)");
        }
        vt_idx = raw_vt - 1;
    }

    std::optional<std::size_t> vn_idx;
    if (!vn_sv.empty()) {
        if (vn_sv[0] == '-') {
            throw std::runtime_error(
                "read_obj: negative (relative) face indices are not supported");
        }
        const auto raw_vn = to_numeric<std::size_t>(vn_sv);
        if (raw_vn == 0) {
            throw std::runtime_error(
                "read_obj: face index 0 is invalid (OBJ indices are 1-based)");
        }
        vn_idx = raw_vn - 1;
    }

    return {v, vt_idx, vn_idx};
}

/**
 * @brief Core OBJ reader — all three tiers share this implementation
 *
 * @param uvmap          Pointer to the UVMap to populate; null for Tier 1
 * @param texture_paths  Pointer to the texture-path vector to populate; null
 *                       for Tiers 1 and 2
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void read_obj_impl(
    const std::filesystem::path& path,
    Mesh<T, Dims, VTraits>& mesh,
    UVMapT* uvmap,
    std::vector<std::filesystem::path>* texture_paths)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error(
            "read_obj: cannot open file: " + path.string());
    }

    // Temporary storage for vn and vt entries (OBJ 1-based pool)
    std::vector<Vec<T, 3>> normals_tmp;
    // vt_tmp[i] is the pool index in *uvmap for OBJ vt index i+1
    // Only used when uvmap != nullptr
    std::vector<std::size_t> vt_to_pool;

    // Material tracking for chart population
    std::unordered_map<std::string, std::size_t> material_index;
    std::size_t cur_material = 0;

    // MTL file path (resolved next to the OBJ)
    std::filesystem::path mtllib_path;

    // Hoisted face-parsing scratch space (avoids per-face heap allocations)
    typename Mesh<T, Dims, VTraits>::Face face_verts;
    std::vector<std::optional<std::size_t>> face_vts;
    std::vector<std::optional<std::size_t>> face_vns;

    std::string line;
    while (std::getline(file, line)) {
        // Strip Windows-style carriage return
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        const auto tokens = split(std::string_view(line));
        if (tokens.empty() || tokens[0].front() == '#') {
            continue;
        }

        // ---- Vertex position ----
        if (tokens[0] == "v") {
            if (tokens.size() < 4) {
                continue;
            }
            T x = to_numeric<T>(tokens[1]);
            T y = to_numeric<T>(tokens[2]);
            T z = to_numeric<T>(tokens[3]);
            const auto vi = mesh.insert_vertex(x, y, z);
            if constexpr (traits::has_color<Vertex>::value) {
                if (tokens.size() >= 7) {
                    Color::F32C3 rgb{
                        to_numeric<float>(tokens[4]),
                        to_numeric<float>(tokens[5]),
                        to_numeric<float>(tokens[6])};
                    mesh.vertex(vi).color = rgb;
                }
            }
        }

        // ---- Vertex normal ----
        else if (tokens[0] == "vn") {
            if (tokens.size() < 4) {
                continue;
            }
            if constexpr (traits::has_normal<Vertex>::value) {
                Vec<T, Dims> n{};
                n[0] = to_numeric<T>(tokens[1]);
                n[1] = to_numeric<T>(tokens[2]);
                n[2] = to_numeric<T>(tokens[3]);
                normals_tmp.push_back(n);
            }
            // Always bump the temp count even when we don't store,
            // so vn indices remain valid if the mesh gains normals later.
            // (If traits::has_normal is false we never read normals_tmp, so it
            //  stays empty and we just never push — indices won't be used.)
        }

        // ---- Texture coordinate ----
        else if (tokens[0] == "vt") {
            if (uvmap == nullptr || tokens.size() < 3) {
                continue;
            }
            const auto u = to_numeric<float>(tokens[1]);
            const auto v = to_numeric<float>(tokens[2]);
            const auto pool_idx = uvmap->insert(u, v);
            vt_to_pool.push_back(pool_idx);
        }

        // ---- Material reference ----
        else if (tokens[0] == "usemtl") {
            if (tokens.size() < 2) {
                continue;
            }
            const std::string name{tokens[1]};
            auto it = material_index.find(name);
            if (it == material_index.end()) {
                const auto idx = material_index.size();
                material_index.emplace(name, idx);
                cur_material = idx;
            } else {
                cur_material = it->second;
            }
        }

        // ---- MTL library reference ----
        else if (tokens[0] == "mtllib") {
            if (texture_paths == nullptr || tokens.size() < 2) {
                continue;
            }
            // Resolve MTL path relative to the OBJ file
            mtllib_path = path.parent_path() / std::string(tokens[1]);
        }

        // ---- Face ----
        else if (tokens[0] == "f") {
            if (tokens.size() < 4) {
                continue;  // degenerate
            }

            face_verts.clear();
            face_vts.clear();
            face_vns.clear();

            for (std::size_t ti = 1; ti < tokens.size(); ++ti) {
                auto [vi, vt, vn] = parse_face_ref(tokens[ti]);
                if (vi >= mesh.num_vertices()) {
                    throw std::runtime_error(
                        "read_obj: face vertex index " +
                        std::to_string(vi + 1) +
                        " out of range (num_vertices=" +
                        std::to_string(mesh.num_vertices()) + ")");
                }
                face_verts.push_back(vi);
                face_vts.push_back(vt);
                face_vns.push_back(vn);
            }

            const auto fi = mesh.insert_face(face_verts);

            // Populate per-vertex normals
            if constexpr (traits::has_normal<Vertex>::value) {
                for (std::size_t ci = 0; ci < face_verts.size(); ++ci) {
                    if (face_vns[ci].has_value()) {
                        const auto ni = *face_vns[ci];
                        if (ni < normals_tmp.size()) {
                            mesh.vertex(face_verts[ci]).normal =
                                normals_tmp[ni];
                        }
                    }
                }
            }

            // Populate per-wedge UVs
            if (uvmap != nullptr) {
                for (std::size_t ci = 0; ci < face_verts.size(); ++ci) {
                    if (face_vts[ci].has_value()) {
                        const auto oi = *face_vts[ci];
                        if (oi < vt_to_pool.size()) {
                            const auto pool_idx = vt_to_pool[oi];
                            uvmap->map(fi, ci, pool_idx);
                            // Populate chart index
                            if constexpr (traits::has_chart<UVMapT>::value) {
                                uvmap->at(pool_idx).chart = cur_material;
                            }
                        }
                    }
                }
            }
        }
    }

    if (file.bad()) {
        throw std::runtime_error("read_obj: I/O error while reading file");
    }

    // Parse MTL for texture paths (Tier 3 only)
    if (texture_paths != nullptr && !mtllib_path.empty()) {
        std::ifstream mtl(mtllib_path);
        if (!mtl) {
            return;  // MTL missing — no-op per spec
        }
        // Collect map_Kd entries in material-declaration order
        // map: material_name -> map_Kd path (in declaration order)
        std::vector<std::filesystem::path> ordered_paths;
        bool in_material = false;
        std::string mtl_line;
        while (std::getline(mtl, mtl_line)) {
            if (!mtl_line.empty() && mtl_line.back() == '\r') {
                mtl_line.pop_back();
            }
            const auto mt = split(std::string_view(mtl_line));
            if (mt.empty() || mt[0].front() == '#') {
                continue;
            }
            if (mt[0] == "newmtl") {
                in_material = true;
                ordered_paths.emplace_back();  // placeholder
            } else if (mt[0] == "map_Kd" && in_material && mt.size() >= 2) {
                if (!ordered_paths.empty()) {
                    ordered_paths.back() = std::filesystem::path{std::string(mt[1])};
                }
            }
        }
        // Remove placeholder entries that had no map_Kd
        for (const auto& p : ordered_paths) {
            if (!p.empty()) {
                texture_paths->push_back(p);
            }
        }
    }
}

/**
 * @brief Write @c v, @c vt, and @c vn lines to an OBJ stream
 *
 * Shared helper used by all write_obj tiers to avoid duplicating the
 * vertex/texture-coord/normal emission code.
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_obj_vertices(
    std::ostream& file,
    std::array<char, 128>& buf,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT* uvmap)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    // Vertex positions (+ optional inline colors)
    for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
        const auto& v = mesh.vertex(vi);
        file << "v " << to_string_view(buf, v[0]) << ' '
                     << to_string_view(buf, v[1]) << ' '
                     << to_string_view(buf, v[2]);
        if constexpr (traits::has_color<Vertex>::value) {
            const auto [r, g, b] = detail::color_to_rgb(v.color);
            file << ' ' << to_string_view(buf, r) << ' '
                        << to_string_view(buf, g) << ' '
                        << to_string_view(buf, b);
        }
        file << '\n';
    }

    // Texture coordinates (pool order) — only when uvmap is provided
    if (uvmap != nullptr) {
        for (std::size_t ui = 0; ui < uvmap->size(); ++ui) {
            const auto& uv = uvmap->at(ui);
            file << "vt " << to_string_view(buf, uv[0]) << ' '
                          << to_string_view(buf, uv[1]) << '\n';
        }
    }

    // Normals — one vn per vertex; vn index equals vertex index (both 1-based)
    if constexpr (traits::has_normal<Vertex>::value) {
        static_assert(Dims == 3, "write_obj: normals require Dims == 3");
        for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
            const auto& v = mesh.vertex(vi);
            const auto n = v.normal.value_or(Vec<T, Dims>{});
            file << "vn " << to_string_view(buf, n[0]) << ' '
                          << to_string_view(buf, n[1]) << ' '
                          << to_string_view(buf, n[2]) << '\n';
        }
    }
}

/**
 * @brief Write face lines to an OBJ stream (Tiers 1–3a)
 *
 * Shared helper for Tier 1 (no UVs), Tier 2, and Tier 3a. Faces are written
 * in mesh order; no @c usemtl grouping is performed here.
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_obj_faces(
    std::ostream& file,
    std::array<char, 128>& buf,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT* uvmap)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        const auto& face = mesh.face(fi);
        file << 'f';
        for (std::size_t ci = 0; ci < face.size(); ++ci) {
            const auto vi = face[ci];
            file << ' ' << to_string_view(buf, vi + 1);
            if (uvmap != nullptr) {
                if (uvmap->has(fi, ci)) {
                    file << '/' << to_string_view(buf, uvmap->get(fi, ci) + 1);
                } else {
                    file << '/';
                }
                if constexpr (traits::has_normal<Vertex>::value) {
                    file << '/' << to_string_view(buf, vi + 1);
                }
            } else {
                if constexpr (traits::has_normal<Vertex>::value) {
                    file << "//" << to_string_view(buf, vi + 1);
                }
            }
        }
        file << '\n';
    }
}

}  // namespace detail

// =============================================================================
// write_obj — Tier 1: positions (+ optional inline colors / normals via traits)
// =============================================================================

/**
 * @brief Write a mesh to an OBJ file
 *
 * Emits one @c v line per vertex; if @c Vertex inherits @ref
 * traits::WithColor, appends inline @c r @c g @c b values in [0,1].
 * If @c Vertex inherits @ref traits::WithNormal, emits @c vn lines (one per
 * vertex, in vertex order) and encodes the normal index in @c f lines as
 * @c "vi//vni".
 *
 * @throws std::runtime_error if the file cannot be opened
 * @tparam T  Mesh numeric type
 * @tparam Dims Must be ≥ 3 (normals require exactly 3)
 * @tparam VTraits Vertex traits type
 */
template <typename T, std::size_t Dims, typename VTraits>
void write_obj(
    const std::filesystem::path& path, const Mesh<T, Dims, VTraits>& mesh)
{
    static_assert(Dims >= 3, "write_obj requires Dims >= 3");

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_obj: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;
    using DummyUV = UVMap<float, 2>;
    DummyUV* no_uvmap = nullptr;
    detail::write_obj_vertices(file, buf, mesh, no_uvmap);
    detail::write_obj_faces(file, buf, mesh, no_uvmap);

    if (!file) {
        throw std::runtime_error(
            "write_obj: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// write_obj — Tier 2: positions + UVMap (+ optional colors / normals)
// =============================================================================

/**
 * @brief Write a mesh and its UV coordinates to an OBJ file
 *
 * In addition to Tier 1 output, emits @c vt lines from the UVMap pool and
 * encodes per-wedge UV indices in @c f lines as @c "vi/vti" (or
 * @c "vi/vti/vni" when normals are present). No @c .mtl file is written.
 *
 * @throws std::runtime_error if the file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_obj(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT& uvmap)
{
    static_assert(Dims >= 3, "write_obj requires Dims >= 3");

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_obj: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;
    detail::write_obj_vertices(file, buf, mesh, &uvmap);
    detail::write_obj_faces(file, buf, mesh, &uvmap);

    if (!file) {
        throw std::runtime_error(
            "write_obj: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// write_obj — Tier 3a: positions + UVMap + single texture path
// =============================================================================

/**
 * @brief Write a mesh, UV map, and a single texture reference to OBJ + MTL
 *
 * Writes @c mtllib <stem>.mtl in the OBJ header, @c usemtl material0 before
 * all face lines, and a companion @c .mtl file containing:
 * @code
 * newmtl material0
 * map_Kd <texture_path>
 * @endcode
 *
 * @throws std::runtime_error if either file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_obj(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT& uvmap,
    const std::filesystem::path& texture_path)
{
    static_assert(Dims >= 3, "write_obj requires Dims >= 3");

    // Write MTL
    const auto mtl_path =
        std::filesystem::path(path).replace_extension(".mtl");
    {
        std::ofstream mtl(mtl_path);
        if (!mtl) {
            throw std::runtime_error(
                "write_obj: cannot open MTL: " + mtl_path.string());
        }
        mtl << "newmtl material0\n"
            << "map_Kd " << texture_path.string() << '\n';
    }

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_obj: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;

    file << "mtllib " << mtl_path.filename().string() << '\n';
    file << "usemtl material0\n";

    detail::write_obj_vertices(file, buf, mesh, &uvmap);
    detail::write_obj_faces(file, buf, mesh, &uvmap);

    if (!file) {
        throw std::runtime_error(
            "write_obj: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// write_obj — Tier 3b: positions + UVMap + multiple texture paths (multi-chart)
// =============================================================================

/**
 * @brief Write a mesh, UV map, and per-chart texture paths to OBJ + MTL
 *
 * Requires @c UVMapT to carry @ref traits::WithChart (enforced via
 * @c static_assert). Faces are grouped by the chart index of their corner-0
 * UV coordinate; each group is preceded by a @c usemtl @c materialN
 * directive. The companion @c .mtl file contains one @c newmtl / @c map_Kd
 * entry per path, in index order.
 *
 * @throws std::runtime_error if either file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_obj(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT& uvmap,
    const std::vector<std::filesystem::path>& texture_paths)
{
    static_assert(
        traits::has_chart<UVMapT>::value,
        "write_obj with multiple texture paths requires UVMap with "
        "traits::WithChart");
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;
    static_assert(Dims >= 3, "write_obj requires Dims >= 3");

    // Write MTL
    const auto mtl_path =
        std::filesystem::path(path).replace_extension(".mtl");
    {
        std::ofstream mtl(mtl_path);
        if (!mtl) {
            throw std::runtime_error(
                "write_obj: cannot open MTL: " + mtl_path.string());
        }
        for (std::size_t i = 0; i < texture_paths.size(); ++i) {
            mtl << "newmtl material" << i << '\n'
                << "map_Kd " << texture_paths[i].string() << '\n';
        }
    }

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_obj: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;

    file << "mtllib " << mtl_path.filename().string() << '\n';

    detail::write_obj_vertices(file, buf, mesh, &uvmap);

    // Group face indices by chart index of corner-0 UV
    std::vector<std::vector<std::size_t>> chart_faces(texture_paths.size());
    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        if (uvmap.has(fi, 0)) {
            const auto chart = uvmap.at(uvmap.get(fi, 0)).chart;
            if (chart < chart_faces.size()) {
                chart_faces[chart].push_back(fi);
            }
        }
    }

    // Write faces grouped by chart
    for (std::size_t ci = 0; ci < texture_paths.size(); ++ci) {
        if (chart_faces[ci].empty()) {
            continue;
        }
        file << "usemtl material" << ci << '\n';
        for (const auto fi : chart_faces[ci]) {
            const auto& face = mesh.face(fi);
            file << 'f';
            for (std::size_t corner = 0; corner < face.size(); ++corner) {
                const auto vi = face[corner];
                file << ' ' << to_string_view(buf, vi + 1);
                if (uvmap.has(fi, corner)) {
                    file << '/'
                         << to_string_view(buf, uvmap.get(fi, corner) + 1);
                } else {
                    file << '/';
                }
                if constexpr (traits::has_normal<Vertex>::value) {
                    file << '/' << to_string_view(buf, vi + 1);
                }
            }
            file << '\n';
        }
    }

    if (!file) {
        throw std::runtime_error(
            "write_obj: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// read_obj — public overloads
// =============================================================================

/**
 * @brief Read an OBJ file into a mesh (Tier 1 — positions only)
 *
 * Parses @c v (with optional inline RGB for @ref traits::WithColor vertices),
 * @c vn (for @ref traits::WithNormal vertices), and @c f lines. All other
 * directives are silently ignored.
 *
 * @throws std::runtime_error if the file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits>
void read_obj(
    const std::filesystem::path& path, Mesh<T, Dims, VTraits>& mesh)
{
    // Use a dummy UVMap type; uvmap pointer is null so it is never accessed
    using DummyUV = UVMap<float, 2>;
    DummyUV* no_uvmap = nullptr;
    detail::read_obj_impl(path, mesh, no_uvmap, nullptr);
}

/**
 * @brief Read an OBJ file into a mesh and UV map (Tier 2)
 *
 * In addition to Tier 1, parses @c vt lines into the UV pool and maps
 * per-wedge UV indices. Chart indices are populated on @c uvmap coordinates
 * when @c UVMapT carries @ref traits::WithChart, using the order in which
 * @c usemtl directives appear.
 *
 * @throws std::runtime_error if the file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void read_obj(
    const std::filesystem::path& path,
    Mesh<T, Dims, VTraits>& mesh,
    UVMapT& uvmap)
{
    detail::read_obj_impl(path, mesh, &uvmap, nullptr);
}

/**
 * @brief Read an OBJ file into a mesh, UV map, and texture path list (Tier 3)
 *
 * In addition to Tier 2, parses the @c .mtl file referenced by the
 * @c mtllib directive and appends @c map_Kd paths (in material-declaration
 * order) to @p texture_paths. If no @c .mtl is referenced or no @c map_Kd
 * entries are present, @p texture_paths is left unchanged.
 *
 * @throws std::runtime_error if the OBJ file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void read_obj(
    const std::filesystem::path& path,
    Mesh<T, Dims, VTraits>& mesh,
    UVMapT& uvmap,
    std::vector<std::filesystem::path>& texture_paths)
{
    detail::read_obj_impl(path, mesh, &uvmap, &texture_paths);
}

}  // namespace educelab

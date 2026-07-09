#pragma once

/** @file */

#include <array>
#include <filesystem>
#include <fstream>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "educelab/core/types/Color.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"
#include "educelab/core/utils/String.hpp"

namespace educelab
{

namespace detail
{

/** @brief Convert a Color to normalized [0,1] float RGB */
inline auto color_to_rgb(const Color& c) -> std::array<float, 3>
{
    switch (c.type()) {
        case Color::Type::F32C1: {
            const auto v = c.value<Color::F32C1>();
            return {v, v, v};
        }
        case Color::Type::F32C3: {
            const auto v = c.value<Color::F32C3>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::F32C4: {
            const auto v = c.value<Color::F32C4>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::U8C1: {
            const auto v = c.value<Color::U8C1>() / 255.f;
            return {v, v, v};
        }
        case Color::Type::U8C3: {
            const auto v = c.value<Color::U8C3>();
            return {v[0] / 255.f, v[1] / 255.f, v[2] / 255.f};
        }
        case Color::Type::U8C4: {
            const auto v = c.value<Color::U8C4>();
            return {v[0] / 255.f, v[1] / 255.f, v[2] / 255.f};
        }
        case Color::Type::U16C1: {
            const auto v = c.value<Color::U16C1>() / 65535.f;
            return {v, v, v};
        }
        case Color::Type::U16C3: {
            const auto v = c.value<Color::U16C3>();
            return {v[0] / 65535.f, v[1] / 65535.f, v[2] / 65535.f};
        }
        case Color::Type::U16C4: {
            const auto v = c.value<Color::U16C4>();
            return {v[0] / 65535.f, v[1] / 65535.f, v[2] / 65535.f};
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
inline auto parse_face_ref(
    std::string_view token, const std::size_t line_no) -> std::
    tuple<std::size_t, std::optional<std::size_t>, std::optional<std::size_t>>
{
    // Find first '/'
    const auto p1 = token.find('/');
    // No-op if v_sv == token if p1 == string_view::npos
    const auto v_sv = token.substr(0, p1);

    // "v" is always present
    if (v_sv.empty()) {
        throw std::runtime_error(
            "read_obj: l." + to_string(line_no - 1) +
            ": empty vertex index is invalid ");
    }
    if (v_sv[0] == '-') {
        throw std::runtime_error(
            "read_obj: l." + to_string(line_no - 1) +
            ": negative (relative) face indices are not supported");
    }
    const auto raw_v = to_numeric<std::size_t>(v_sv);
    if (raw_v == 0) {
        throw std::runtime_error(
            "read_obj: l." + to_string(line_no - 1) +
            ": vertex index 0 is invalid (OBJ indices are 1-based)");
    }
    const auto v = raw_v - 1;
    if (p1 == std::string_view::npos) {
        return {v, std::nullopt, std::nullopt};
    }
    const auto rest = token.substr(p1 + 1);

    // Find second '/'
    const auto p2 = rest.find('/');
    if (p2 == std::string_view::npos) {
        // "v/vt"
        if (rest.empty()) {
            // "v/"
            return {v, std::nullopt, std::nullopt};
        }
        if (rest[0] == '-') {
            throw std::runtime_error(
                "read_obj: l." + to_string(line_no - 1) +
                ": negative (relative) face indices are not supported");
        }
        const auto raw_vt = to_numeric<std::size_t>(rest);
        if (raw_vt == 0) {
            throw std::runtime_error(
                "read_obj: l." + to_string(line_no - 1) +
                ": face index 0 is invalid (OBJ indices are 1-based)");
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
                "read_obj: l." + to_string(line_no - 1) +
                ": negative (relative) face indices are not supported");
        }
        const auto raw_vt = to_numeric<std::size_t>(vt_sv);
        if (raw_vt == 0) {
            throw std::runtime_error(
                "read_obj: l." + to_string(line_no - 1) +
                ": face index 0 is invalid (OBJ indices are 1-based)");
        }
        vt_idx = raw_vt - 1;
    }

    std::optional<std::size_t> vn_idx;
    if (!vn_sv.empty()) {
        if (vn_sv[0] == '-') {
            throw std::runtime_error(
                "read_obj: l." + to_string(line_no - 1) +
                ": negative (relative) face indices are not supported");
        }
        const auto raw_vn = to_numeric<std::size_t>(vn_sv);
        if (raw_vn == 0) {
            throw std::runtime_error(
                "read_obj: l." + to_string(line_no - 1) +
                ": face index 0 is invalid (OBJ indices are 1-based)");
        }
        vn_idx = raw_vn - 1;
    }

    return {v, vt_idx, vn_idx};
}

/**
 * @brief Core OBJ reader — all three tiers share this implementation
 *
 * @param path           Input file path
 * @param mesh           Output mesh
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

    // Clear the outputs before constructing
    mesh.clear();
    if (uvmap) {
        uvmap->clear();
    }
    if (texture_paths) {
        texture_paths->clear();
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

    // Hoisted face-parsing scratch space
    using Face = typename Mesh<T, Dims, VTraits>::Face;
    struct FaceReference {
        Face vs;
        std::vector<std::optional<std::size_t>> vts;
        std::vector<std::optional<std::size_t>> vns;
        std::size_t material = 0;
    };
    std::vector<FaceReference> face_references;

    std::string line;
    std::vector<std::string_view> tokens;
    std::size_t line_no{0};
    while (std::getline(file, line)) {
        line_no++;
        split(std::string_view(line), tokens);
        if (tokens.empty() || tokens[0].front() == '#') {
            continue;
        }

        // ---- Vertex position ----
        if (tokens[0] == "v") {
            if (tokens.size() < 4) {
                throw std::runtime_error(
                    "read_obj: l." + to_string(line_no - 1) +
                    ": invalid v declaration");
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
                throw std::runtime_error(
                    "read_obj: l." + to_string(line_no - 1) +
                    ": invalid vn declaration");
            }
            if constexpr (traits::has_normal<Vertex>::value) {
                Vec<T, Dims> n{};
                n[0] = to_numeric<T>(tokens[1]);
                n[1] = to_numeric<T>(tokens[2]);
                n[2] = to_numeric<T>(tokens[3]);
                normals_tmp.push_back(n);
            }
        }

        // ---- Texture coordinate ----
        else if (tokens[0] == "vt") {
            if (uvmap == nullptr) {
                continue;
            }
            if (tokens.size() < 3) {
                throw std::runtime_error(
                    "read_obj: l." + to_string(line_no - 1) +
                    ": invalid vt declaration");
            }
            const auto u = to_numeric<float>(tokens[1]);
            const auto v = to_numeric<float>(tokens[2]);
            const auto pool_idx = uvmap->insert(u, v);
            vt_to_pool.push_back(pool_idx);
        }

        // ---- Material reference ----
        else if (tokens[0] == "usemtl") {
            if (tokens.size() < 2) {
                throw std::runtime_error(
                    "read_obj: l." + to_string(line_no - 1) +
                    ": invalid usemtl declaration");
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
            if (texture_paths == nullptr) {
                continue;
            }
            if (tokens.size() < 2) {
                throw std::runtime_error(
                    "read_obj: l." + to_string(line_no - 1) +
                    ": invalid mtllib declaration");
            }
            // Resolve MTL path relative to the OBJ file. Capture everything
            // starting with the first token in case the filename has spaces.
            const auto name_pos = std::string_view(line).find(tokens[1]);
            mtllib_path = path.parent_path() / line.substr(name_pos);
        }

        // ---- Face ----
        else if (tokens[0] == "f") {
            if (tokens.size() < 4) {
                throw std::runtime_error(
                    "read_obj: l." + to_string(line_no - 1) +
                    ": invalid f declaration");
            }

            FaceReference face;
            for (std::size_t ti = 1; ti < tokens.size(); ++ti) {
                auto [vi, vt, vn] = parse_face_ref(tokens[ti], line_no);
                face.vs.push_back(vi);
                face.vts.push_back(vt);
                face.vns.push_back(vn);
            }
            face.material = cur_material;
            face_references.push_back(face);
        }
    }

    // Build faces and uv map after parsing all vs/vts/vns
    for (const auto& face : face_references) {
        // Check the face vertices now that all have been parsed
        for (const auto& vi : face.vs) {
            if (vi >= mesh.num_vertices()) {
                throw std::runtime_error(
                    "read_obj: face vertex index " + std::to_string(vi + 1) +
                    " out of range (num_vertices=" +
                    std::to_string(mesh.num_vertices()) + ")");
            }
        }
        const auto fi = mesh.insert_face(face.vs);

        if constexpr (traits::has_normal<Vertex>::value) {
            for (std::size_t ci = 0; ci < face.vs.size(); ++ci) {
                if (!face.vns[ci].has_value()) {
                    continue;
                }
                const auto ni = *face.vns[ci];
                if (ni >= normals_tmp.size()) {
                    throw std::runtime_error(
                        "read_obj: face normal index " +
                        std::to_string(ni + 1) + " out of range (num_normals=" +
                        std::to_string(normals_tmp.size()) + ")");
                }
                mesh.vertex(face.vs[ci]).normal = normals_tmp[ni];
            }
        }

        // Populate per-wedge UVs
        if (uvmap != nullptr) {
            // For each face wedge
            for (std::size_t ci = 0; ci < face.vs.size(); ++ci) {
                // Skip edges without UV coordinates
                if (!face.vts[ci].has_value()) {
                    continue;
                }
                // Get the vt-to-uv pool index
                const auto oi = *face.vts[ci];
                if (oi >= vt_to_pool.size()) {
                    continue;
                }
                const auto pool_idx = vt_to_pool[oi];
                // Add the
                uvmap->map(fi, ci, pool_idx);
                // Populate chart index
                if constexpr (traits::has_chart<UVMapT>::value) {
                    uvmap->at(pool_idx).chart = face.material;
                }
            }
        }
    }

    // Did we close cleanly?
    if (file.bad()) {
        throw std::runtime_error("read_obj: I/O error while reading file");
    }

    // Parse MTL for texture paths (Tier 3 only)
    if (texture_paths == nullptr || mtllib_path.empty()) {
        return;
    }
    std::ifstream mtl(mtllib_path);
    if (!mtl) {
        return;  // MTL missing / unreadable — leave texture_paths empty
    }

    // Collect (material name -> map_Kd path) in MTL declaration order.
    std::vector<std::pair<std::string, std::filesystem::path>> mtl_materials;
    std::string mtl_line;
    std::vector<std::string_view> mt;
    while (std::getline(mtl, mtl_line)) {
        split(mtl_line, mt);
        if (mt.empty() || mt[0].front() == '#') {
            continue;
        }
        if (mt[0] == "newmtl" && mt.size() >= 2) {
            mtl_materials.emplace_back(
                std::string(mt[1]), std::filesystem::path{});
        } else if (
            mt[0] == "map_Kd" && !mtl_materials.empty() && mt.size() >= 2) {
            // Capture everything from the first argument to end of line so
            // texture filenames may contain spaces (mirrors the mtllib
            // handling above). trim() drops any trailing whitespace / CR.
            const auto arg_pos = std::string_view(mtl_line).find(mt[1]);
            mtl_materials.back().second = std::filesystem::path{
                std::string(trim(std::string_view(mtl_line).substr(arg_pos)))};
        }
    }

    if (!material_index.empty()) {
        // Chart indices are assigned in usemtl *usage* order, which can differ
        // from the MTL's newmtl *declaration* order. Align texture_paths with
        // chart indices by looking each material up by name, so
        // texture_paths[chart] is that chart's image. Materials with no map_Kd
        // (or absent from the MTL) keep an empty path so index alignment holds.
        texture_paths->assign(material_index.size(), std::filesystem::path{});
        for (const auto& [name, tex] : mtl_materials) {
            if (auto it = material_index.find(name);
                it != material_index.end()) {
                (*texture_paths)[it->second] = tex;
            }
        }
    } else {
        // No usemtl directives: emit each map_Kd in declaration order (legacy
        // single-/implicit-material behavior).
        for (const auto& [name, tex] : mtl_materials) {
            if (!tex.empty()) {
                texture_paths->push_back(tex);
            }
        }
    }
}

/**
 * @brief Build a compact @c vn pool: map each vertex to its 1-based @c vn
 * index, assigned in vertex order to only those vertices that carry a normal
 *
 * The OBJ @c vn pool is independent of the @c v pool, so there is no need to
 * emit one @c vn per vertex. Vertices without a normal are mapped to @c 0
 * ("no @c vn ref") and contribute no @c vn line — a normal-less mesh emits no
 * @c vn at all (no fabricated @c "vn 0 0 0"), and a partially-normalled mesh
 * references normals only where they exist. Returns an empty vector when the
 * vertex type carries no normals.
 */
template <typename T, std::size_t Dims, typename VTraits>
std::vector<std::size_t> build_normal_index(const Mesh<T, Dims, VTraits>& mesh)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;
    std::vector<std::size_t> vn_index;
    if constexpr (traits::has_normal<Vertex>::value) {
        vn_index.assign(mesh.num_vertices(), 0);
        std::size_t next = 0;
        for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
            if (mesh.vertex(vi).normal.has_value()) {
                vn_index[vi] = ++next;  // 1-based pool index
            }
        }
    }
    return vn_index;
}

/**
 * @brief Write @c v, @c vt, and @c vn lines to an OBJ stream
 *
 * Shared helper used by all write_obj tiers to avoid duplicating the
 * vertex/texture-coord/normal emission code. @c vn_index (from
 * @ref build_normal_index) selects which vertices contribute a @c vn line and
 * in what pool order.
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_obj_vertices(
    std::ostream& file,
    std::array<char, 128>& buf,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT* uvmap,
    [[maybe_unused]] const std::vector<std::size_t>& vn_index)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    // Vertex positions (+ optional inline colors)
    for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
        const auto& v = mesh.vertex(vi);
        file << "v " << to_string_view(buf, v[0]) << ' '
                     << to_string_view(buf, v[1]) << ' '
                     << to_string_view(buf, v[2]);
        // Inline RGB only for vertices that carry a color; a color-less vertex
        // emits "v x y z" so the reader leaves its color unset (no fabricated
        // black). The OBJ reader detects color per line via token count.
        if constexpr (traits::has_color<Vertex>::value) {
            if (v.color.has_value()) {
                const auto [r, g, b] = detail::color_to_rgb(v.color);
                file << ' ' << to_string_view(buf, r) << ' '
                            << to_string_view(buf, g) << ' '
                            << to_string_view(buf, b);
            }
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

    // Normals — compact pool, in vertex order, only for vertices that carry a
    // normal (vn_index[vi] != 0). A normal-less mesh emits no vn lines.
    if constexpr (traits::has_normal<Vertex>::value) {
        static_assert(Dims == 3, "write_obj: normals require Dims == 3");
        for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
            if (vn_index[vi] == 0) {
                continue;
            }
            const auto& n = *mesh.vertex(vi).normal;
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
    const UVMapT* uvmap,
    [[maybe_unused]] const std::vector<std::size_t>& vn_index)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        const auto& face = mesh.face(fi);
        file << 'f';
        for (std::size_t ci = 0; ci < face.size(); ++ci) {
            const auto vi = face[ci];
            file << ' ' << to_string_view(buf, vi + 1);
            if (uvmap != nullptr && uvmap->has(fi, ci)) {
                file << '/' << to_string_view(buf, uvmap->get(fi, ci) + 1);
                if constexpr (traits::has_normal<Vertex>::value) {
                    if (vn_index[vi] != 0) {
                        file << '/' << to_string_view(buf, vn_index[vi]);
                    }
                }
            } else if constexpr (traits::has_normal<Vertex>::value) {
                if (vn_index[vi] != 0) {
                    file << "//" << to_string_view(buf, vn_index[vi]);
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
 * traits::WithColor, appends inline @c r @c g @c b values in [0,1] for each
 * vertex that carries a color (color-less vertices emit @c "v x y z" only).
 * If @c Vertex inherits @ref traits::WithNormal, emits a @c vn line for each
 * vertex that carries a normal (a compact pool in vertex order) and encodes
 * the corresponding @c vn index in @c f lines as @c "vi//vni"; vertices
 * without a normal contribute no @c vn line and no normal ref.
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

    std::array<char, 128> buf{};
    using DummyUV = UVMap<T>;
    DummyUV* no_uvmap = nullptr;
    const auto vn_index = detail::build_normal_index(mesh);
    detail::write_obj_vertices(file, buf, mesh, no_uvmap, vn_index);
    detail::write_obj_faces(file, buf, mesh, no_uvmap, vn_index);

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

    std::array<char, 128> buf{};
    const auto vn_index = detail::build_normal_index(mesh);
    detail::write_obj_vertices(file, buf, mesh, &uvmap, vn_index);
    detail::write_obj_faces(file, buf, mesh, &uvmap, vn_index);

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
 * Writes @c mtllib &lt;stem&gt;.mtl in the OBJ header, @c usemtl material0 before
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

    std::array<char, 128> buf{};

    file << "mtllib " << mtl_path.filename().string() << '\n';
    file << "usemtl material0\n";

    const auto vn_index = detail::build_normal_index(mesh);
    detail::write_obj_vertices(file, buf, mesh, &uvmap, vn_index);
    detail::write_obj_faces(file, buf, mesh, &uvmap, vn_index);

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

    std::array<char, 128> buf{};

    file << "mtllib " << mtl_path.filename().string() << '\n';

    const auto vn_index = detail::build_normal_index(mesh);
    detail::write_obj_vertices(file, buf, mesh, &uvmap, vn_index);

    // Group face indices by chart index of corner-0 UV
    std::vector<std::vector<std::size_t>> chart_faces(texture_paths.size());
    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        if (not uvmap.has(fi, 0)) {
            continue;
        }
        const auto chart = uvmap.at(uvmap.get(fi, 0)).chart;
        if (chart >= chart_faces.size()) {
            continue;
        }
        chart_faces[chart].push_back(fi);
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
                    if constexpr (traits::has_normal<Vertex>::value) {
                        if (vn_index[vi] != 0) {
                            file << '/' << to_string_view(buf, vn_index[vi]);
                        }
                    }
                } else if constexpr (traits::has_normal<Vertex>::value) {
                    if (vn_index[vi] != 0) {
                        file << "//" << to_string_view(buf, vn_index[vi]);
                    }
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
    using DummyUV = UVMap<T, 2>;
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
 * @c mtllib directive and populates @p texture_paths indexed by UV **chart
 * index** (i.e. @c usemtl usage order), resolving each material by name so
 * @c texture_paths[chart] is that chart's @c map_Kd image. Any chart whose
 * material has no @c map_Kd (or is absent from the @c .mtl) gets an empty
 * path so indexing by chart stays valid. If no @c .mtl is referenced, or the
 * referenced @c .mtl cannot be opened, @p texture_paths is left empty.
 *
 * A @c map_Kd value captures the rest of the line (trimmed), so texture
 * filenames may contain spaces. A @c newmtl with no material name is ignored,
 * along with any @c map_Kd that would attach to it.
 *
 * If the OBJ contains no @c usemtl directives, each @c map_Kd is appended in
 * @c .mtl declaration order instead (legacy single-/implicit-material meshes).
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

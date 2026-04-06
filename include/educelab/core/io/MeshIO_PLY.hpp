#pragma once

/** @file */

#include <array>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "educelab/core/types/Color.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"
#include "educelab/core/utils/MeshUtils.hpp"
#include "educelab/core/utils/String.hpp"

namespace educelab
{

namespace detail
{

/** @brief Convert a Color to {r, g, b} in [0, 255] uint8 range */
inline auto color_to_u8c3(const Color& c) -> std::array<uint8_t, 3>
{
    switch (c.type()) {
        case Color::Type::U8C3: {
            const auto v = c.value<Color::U8C3>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::U8C4: {
            const auto v = c.value<Color::U8C4>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::F32C3: {
            const auto v = c.value<Color::F32C3>();
            // TODO: Why not use std::round or std::ceil?
            return {
                static_cast<uint8_t>(v[0] * 255.f + 0.5f),
                static_cast<uint8_t>(v[1] * 255.f + 0.5f),
                static_cast<uint8_t>(v[2] * 255.f + 0.5f)};
        }
        case Color::Type::F32C4: {
            const auto v = c.value<Color::F32C4>();
            return {
                static_cast<uint8_t>(v[0] * 255.f + 0.5f),
                static_cast<uint8_t>(v[1] * 255.f + 0.5f),
                static_cast<uint8_t>(v[2] * 255.f + 0.5f)};
        }
        case Color::Type::U16C3: {
            const auto v = c.value<Color::U16C3>();
            return {
                static_cast<uint8_t>(v[0] / 65535.f * 255.f + 0.5f),
                static_cast<uint8_t>(v[1] / 65535.f * 255.f + 0.5f),
                static_cast<uint8_t>(v[2] / 65535.f * 255.f + 0.5f)};
        }
        case Color::Type::U16C4: {
            const auto v = c.value<Color::U16C4>();
            return {
                static_cast<uint8_t>(v[0] / 65535.f * 255.f + 0.5f),
                static_cast<uint8_t>(v[1] / 65535.f * 255.f + 0.5f),
                static_cast<uint8_t>(v[2] / 65535.f * 255.f + 0.5f)};
        }
        case Color::Type::U8C1: {
            const auto v = c.value<Color::U8C1>();
            return {v, v, v};
        }
        case Color::Type::U16C1: {
            const auto v =
                static_cast<uint8_t>(c.value<Color::U16C1>() / 65535.f * 255.f + 0.5f);
            return {v, v, v};
        }
        case Color::Type::F32C1: {
            const auto v = static_cast<uint8_t>(c.value<Color::F32C1>() * 255.f + 0.5f);
            return {v, v, v};
        }
        default:
            return {0, 0, 0};
    }
}

// -------------------------------------------------------------------------
// PLY header description
// -------------------------------------------------------------------------

struct PLYProp {
    std::string name;
    std::string type;  // "float", "double", "int", "uint", "uchar", etc.
};

struct PLYHeader {
    enum class Format { ASCII, BinaryLE, BinaryBE };
    Format format{Format::ASCII};
    std::size_t n_vertices{0};
    std::size_t n_faces{0};
    std::vector<PLYProp> vertex_props;
    std::string face_count_type{"uchar"};
    std::string face_index_type{"int"};
    std::vector<std::filesystem::path> texture_files;
};

inline auto ply_type_bytes(const std::string& t) -> std::size_t
{
    if (t == "float" || t == "int" || t == "uint") return 4;
    if (t == "double") return 8;
    if (t == "short" || t == "ushort") return 2;
    if (t == "char" || t == "uchar") return 1;
    return 4;  // safe fallback
}

/** @brief Parse a PLY header; leaves @p file positioned at first data byte */
inline auto parse_ply_header(std::istream& file) -> PLYHeader
{
    PLYHeader h;
    std::string line;

    // Magic
    if (!std::getline(file, line) || line.rfind("ply", 0) != 0) {
        throw std::runtime_error("read_ply: not a PLY file");
    }

    std::string cur_element;
    bool in_vertex = false;
    bool in_face   = false;
    std::vector<std::string_view> tokens;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        split(std::string_view(line), tokens);
        if (tokens.empty()) {
            continue;
        }

        if (tokens[0] == "end_header") {
            break;
        }
        if (tokens[0] == "format") {
            if (tokens.size() >= 2) {
                if (tokens[1] == "binary_little_endian") {
                    h.format = PLYHeader::Format::BinaryLE;
                } else if (tokens[1] == "binary_big_endian") {
                    h.format = PLYHeader::Format::BinaryBE;
                }
                // else ASCII (default)
            }
        } else if (tokens[0] == "comment") {
            // Look for "comment TextureFile <path>"
            if (tokens.size() >= 3 && tokens[1] == "TextureFile") {
                h.texture_files.emplace_back(std::string(tokens[2]));
            }
        } else if (tokens[0] == "element") {
            if (tokens.size() >= 3) {
                cur_element = std::string(tokens[1]);
                in_vertex   = (cur_element == "vertex");
                in_face     = (cur_element == "face");
                const auto n = to_numeric<std::size_t>(tokens[2]);
                constexpr std::size_t kMaxElements = 500'000'000;
                if (n > kMaxElements) {
                    throw std::runtime_error(
                        "read_ply: element count " + std::to_string(n) +
                        " exceeds safety limit of " +
                        std::to_string(kMaxElements));
                }
                if (in_vertex) {
                    h.n_vertices = n;
                } else if (in_face) {
                    h.n_faces = n;
                }
            }
        } else if (tokens[0] == "property") {
            if (in_vertex && tokens.size() >= 3) {
                if (tokens[1] == "list") {
                    // Ignore list properties on vertex (rare)
                } else {
                    h.vertex_props.push_back(
                        {std::string(tokens[2]), std::string(tokens[1])});
                }
            } else if (in_face && tokens.size() >= 5 && tokens[1] == "list") {
                h.face_count_type = std::string(tokens[2]);
                h.face_index_type = std::string(tokens[3]);
            }
        }
    }
    return h;
}

// -------------------------------------------------------------------------
// Binary property reading (little-endian)
// -------------------------------------------------------------------------

template <typename DestT>
auto read_ply_binary_prop(std::istream& f, const std::string& type) -> DestT
{
    if (type == "float") {
        float v{};
        f.read(reinterpret_cast<char*>(&v), 4);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    if (type == "double") {
        double v{};
        f.read(reinterpret_cast<char*>(&v), 8);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    if (type == "int") {
        int32_t v{};
        f.read(reinterpret_cast<char*>(&v), 4);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    if (type == "uint") {
        uint32_t v{};
        f.read(reinterpret_cast<char*>(&v), 4);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    if (type == "short") {
        int16_t v{};
        f.read(reinterpret_cast<char*>(&v), 2);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    if (type == "ushort") {
        uint16_t v{};
        f.read(reinterpret_cast<char*>(&v), 2);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    if (type == "uchar" || type == "char") {
        uint8_t v{};
        f.read(reinterpret_cast<char*>(&v), 1);
        if (!f) { throw std::runtime_error("read_ply: unexpected end of binary data"); }
        return static_cast<DestT>(v);
    }
    // Unknown: skip 4 bytes
    f.ignore(4);
    return DestT{};
}

// -------------------------------------------------------------------------
// Core PLY reader
// -------------------------------------------------------------------------

template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void read_ply_impl(
    const std::filesystem::path& path,
    Mesh<T, Dims, VTraits>& mesh,
    UVMapT* uvmap,
    std::vector<std::filesystem::path>* texture_paths)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error(
            "read_ply: cannot open file: " + path.string());
    }

    // Clear the outputs before constructing
    mesh.clear();
    if (uvmap) {
        uvmap->clear();
    }
    if (texture_paths) {
        texture_paths->clear();
    }

    const auto hdr = parse_ply_header(file);
    if (hdr.format == PLYHeader::Format::BinaryBE) {
        throw std::runtime_error(
            "read_ply: binary big-endian format is not supported");
    }
    const bool binary = hdr.format == PLYHeader::Format::BinaryLE;

    // Populate texture paths from header
    if (texture_paths != nullptr) {
        for (const auto& tp : hdr.texture_files) {
            texture_paths->push_back(tp);
        }
    }

    // Identify vertex property roles
    bool has_nx{false}, has_ny{false}, has_nz{false};
    bool has_r{false}, has_g{false}, has_b{false};
    bool has_s{false}, has_t{false};
    for (const auto& p : hdr.vertex_props) {
        if (p.name == "nx") has_nx = true;
        if (p.name == "ny") has_ny = true;
        if (p.name == "nz") has_nz = true;
        if (p.name == "red") has_r = true;
        if (p.name == "green") has_g = true;
        if (p.name == "blue") has_b = true;
        if (p.name == "s") has_s = true;
        if (p.name == "t") has_t = true;
    }

    // Temporary per-vertex UV storage (used when uvmap != nullptr)
    // Indexed by vertex index; populated during vertex reading
    std::vector<float> uv_s, uv_t;
    if (uvmap != nullptr && has_s && has_t) {
        uv_s.resize(hdr.n_vertices, 0.f);
        uv_t.resize(hdr.n_vertices, 0.f);
    }

    // Read vertices
    std::vector<std::string_view> tokens;
    for (std::size_t vi = 0; vi < hdr.n_vertices; ++vi) {
        T x{}, y{}, z{};
        T nx{}, ny{}, nz{};
        float r{}, g{}, b{};
        float s{}, t{};

        if (binary) {
            for (const auto& prop : hdr.vertex_props) {
                if (prop.name == "x")       x  = read_ply_binary_prop<T>(file, prop.type);
                else if (prop.name == "y")  y  = read_ply_binary_prop<T>(file, prop.type);
                else if (prop.name == "z")  z  = read_ply_binary_prop<T>(file, prop.type);
                else if (prop.name == "nx") nx = read_ply_binary_prop<T>(file, prop.type);
                else if (prop.name == "ny") ny = read_ply_binary_prop<T>(file, prop.type);
                else if (prop.name == "nz") nz = read_ply_binary_prop<T>(file, prop.type);
                else if (prop.name == "red")   r = read_ply_binary_prop<float>(file, prop.type);
                else if (prop.name == "green") g = read_ply_binary_prop<float>(file, prop.type);
                else if (prop.name == "blue")  b = read_ply_binary_prop<float>(file, prop.type);
                else if (prop.name == "s") s = read_ply_binary_prop<float>(file, prop.type);
                else if (prop.name == "t") t = read_ply_binary_prop<float>(file, prop.type);
                else {
                    // Unknown — skip bytes
                    file.ignore(static_cast<std::streamsize>(
                        ply_type_bytes(prop.type)));
                }
            }
        } else {
            // ASCII
            std::string vline;
            while (std::getline(file, vline)) {
                if (!vline.empty() && vline.back() == '\r') vline.pop_back();
                if (!vline.empty() && vline.front() != '#') break;
            }
            split(std::string_view(vline), tokens);
            for (std::size_t pi = 0;
                 pi < hdr.vertex_props.size() && pi < tokens.size(); ++pi) {
                const auto& prop = hdr.vertex_props[pi];
                if (prop.name == "x")
                    x = to_numeric<T>(tokens[pi]);
                else if (prop.name == "y")
                    y = to_numeric<T>(tokens[pi]);
                else if (prop.name == "z")
                    z = to_numeric<T>(tokens[pi]);
                else if (prop.name == "nx")
                    nx = to_numeric<T>(tokens[pi]);
                else if (prop.name == "ny")
                    ny = to_numeric<T>(tokens[pi]);
                else if (prop.name == "nz")
                    nz = to_numeric<T>(tokens[pi]);
                else if (prop.name == "red")
                    r = to_numeric<float>(tokens[pi]);
                else if (prop.name == "green")
                    g = to_numeric<float>(tokens[pi]);
                else if (prop.name == "blue")
                    b = to_numeric<float>(tokens[pi]);
                else if (prop.name == "s")
                    s = to_numeric<float>(tokens[pi]);
                else if (prop.name == "t")
                    t = to_numeric<float>(tokens[pi]);
            }
        }

        const auto new_vi = mesh.insert_vertex(x, y, z);

        if constexpr (traits::has_normal<Vertex>::value) {
            if (has_nx && has_ny && has_nz) {
                Vec<T, Dims> n{};
                n[0] = nx; n[1] = ny; n[2] = nz;
                mesh.vertex(new_vi).normal = n;
            }
        }
        if constexpr (traits::has_color<Vertex>::value) {
            if (has_r && has_g && has_b) {
                // PLY stores colors as uchar (0-255); read back as such
                mesh.vertex(new_vi).color = Color::U8C3{
                    static_cast<uint8_t>(r),
                    static_cast<uint8_t>(g),
                    static_cast<uint8_t>(b)};
            }
        }
        if (uvmap != nullptr && has_s && has_t) {
            uv_s[vi] = s;
            uv_t[vi] = t;
        }
    }

    // Insert UV pool entries (one per vertex, indexed by vertex index)
    if (uvmap != nullptr && has_s && has_t) {
        for (std::size_t vi = 0; vi < hdr.n_vertices; ++vi) {
            (void)uvmap->insert(uv_s[vi], uv_t[vi]);
        }
    }

    // Read faces
    for (std::size_t fi = 0; fi < hdr.n_faces; ++fi) {
        typename Mesh<T, Dims, VTraits>::Face face;

        if (binary) {
            // Count
            const auto count = read_ply_binary_prop<std::size_t>(
                file, hdr.face_count_type);
            if (count > 256) {
                throw std::runtime_error(
                    "read_ply: face vertex count " + std::to_string(count) +
                    " exceeds maximum of 256");
            }
            face.reserve(count);
            for (std::size_t k = 0; k < count; ++k) {
                const auto idx = read_ply_binary_prop<std::size_t>(
                    file, hdr.face_index_type);
                if (idx >= hdr.n_vertices) {
                    throw std::runtime_error(
                        "read_ply: face vertex index " + std::to_string(idx) +
                        " out of range (n_vertices=" +
                        std::to_string(hdr.n_vertices) + ")");
                }
                face.push_back(idx);
            }
        } else {
            std::string fline;
            while (std::getline(file, fline)) {
                if (!fline.empty() && fline.back() == '\r') fline.pop_back();
                if (!fline.empty() && fline.front() != '#') break;
            }
            split(std::string_view(fline), tokens);
            if (tokens.empty())
                continue;
            const auto count = to_numeric<std::size_t>(tokens[0]);
            if (count > 256) {
                throw std::runtime_error(
                    "read_ply: face vertex count " + std::to_string(count) +
                    " exceeds maximum of 256");
            }
            face.reserve(count);
            for (std::size_t k = 1; k <= count && k < tokens.size(); ++k) {
                const auto idx = to_numeric<std::size_t>(tokens[k]);
                if (idx >= hdr.n_vertices) {
                    throw std::runtime_error(
                        "read_ply: face vertex index " + std::to_string(idx) +
                        " out of range (n_vertices=" +
                        std::to_string(hdr.n_vertices) + ")");
                }
                face.push_back(idx);
            }
        }

        const auto new_fi = mesh.insert_face(face);

        // Map per-vertex UVs to per-wedge entries
        if (uvmap != nullptr && has_s && has_t) {
            for (std::size_t ci = 0; ci < face.size(); ++ci) {
                // Pool index equals vertex index (inserted in vertex order above)
                uvmap->map(new_fi, ci, face[ci]);
            }
        }
    }
}

/**
 * @brief Write the PLY ASCII header to @p file
 *
 * Shared by all write_ply tiers. The @p texture_comment parameter may be
 * empty (no @c comment TextureFile line) or contain a path string.
 * The @p has_uvs flag controls whether @c s and @c t properties are declared.
 */
template <typename T, std::size_t Dims, typename VTraits>
void write_ply_header(
    std::ostream& file,
    const Mesh<T, Dims, VTraits>& mesh,
    const std::string& texture_comment,
    bool has_uvs)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    file << "ply\n"
         << "format ascii 1.0\n";

    if (!texture_comment.empty()) {
        file << "comment TextureFile " << texture_comment << '\n';
    }

    file << "element vertex " << mesh.num_vertices() << '\n'
         << "property float x\n"
         << "property float y\n"
         << "property float z\n";

    if constexpr (traits::has_normal<Vertex>::value) {
        static_assert(Dims == 3, "write_ply: normals require Dims == 3");
        file << "property float nx\n"
             << "property float ny\n"
             << "property float nz\n";
    }
    if constexpr (traits::has_color<Vertex>::value) {
        file << "property uchar red\n"
             << "property uchar green\n"
             << "property uchar blue\n";
    }

    if (has_uvs) {
        file << "property float s\n"
             << "property float t\n";
    }

    file << "element face " << mesh.num_faces() << '\n'
         << "property list uchar int vertex_indices\n"
         << "end_header\n";
}

/**
 * @brief Write PLY ASCII vertex and face data to @p file
 *
 * @p flat_uvs may be empty (no UV output) or have one entry per vertex.
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVVec>
void write_ply_data(
    std::ostream& file,
    std::array<char, 128>& buf,
    const Mesh<T, Dims, VTraits>& mesh,
    const std::vector<UVVec>& flat_uvs)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
        const auto& v = mesh.vertex(vi);
        file << to_string_view(buf, v[0]) << ' '
             << to_string_view(buf, v[1]) << ' '
             << to_string_view(buf, v[2]);
        if constexpr (traits::has_normal<Vertex>::value) {
            const auto n = v.normal.value_or(Vec<T, Dims>{});
            file << ' ' << to_string_view(buf, n[0])
                 << ' ' << to_string_view(buf, n[1])
                 << ' ' << to_string_view(buf, n[2]);
        }
        if constexpr (traits::has_color<Vertex>::value) {
            const auto [r, g, b] = detail::color_to_u8c3(v.color);
            file << ' ' << to_string_view(buf, r)
                 << ' ' << to_string_view(buf, g)
                 << ' ' << to_string_view(buf, b);
        }
        if (!flat_uvs.empty()) {
            const auto& uv = flat_uvs[vi];
            file << ' ' << to_string_view(buf, uv[0])
                 << ' ' << to_string_view(buf, uv[1]);
        }
        file << '\n';
    }

    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        const auto& face = mesh.face(fi);
        file << face.size();
        for (const auto vi : face) {
            file << ' ' << to_string_view(buf, vi);
        }
        file << '\n';
    }
}

}  // namespace detail

// =============================================================================
// write_ply — Tier 1: positions (+ optional normals / colors via traits)
// =============================================================================

/**
 * @brief Write a mesh to an ASCII PLY file
 *
 * Emits @c x @c y @c z vertex properties. If @c Vertex carries
 * @ref traits::WithNormal, also emits @c nx @c ny @c nz properties.
 * If @c Vertex carries @ref traits::WithColor, also emits @c red @c green
 * @c blue properties (@c uchar, 0–255).
 *
 * @throws std::runtime_error if the file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits>
void write_ply(
    const std::filesystem::path& path, const Mesh<T, Dims, VTraits>& mesh)
{
    static_assert(Dims >= 3, "write_ply requires Dims >= 3");

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_ply: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;
    using UVVec = Vec<float, 2>;
    const std::vector<UVVec> no_uvs;
    detail::write_ply_header(file, mesh, "", false);
    detail::write_ply_data(file, buf, mesh, no_uvs);

    if (!file) {
        throw std::runtime_error(
            "write_ply: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// write_ply — Tier 2: positions + UVMap (seam expansion)
// =============================================================================

/**
 * @brief Write a mesh and UV map to an ASCII PLY file
 *
 * Calls @ref expand_at_seams to produce a per-vertex UV array before writing.
 * The expanded vertex count may be larger than the original if UV seams are
 * present. Adds @c s and @c t vertex properties. No @c comment @c TextureFile
 * line is written.
 *
 * @throws std::runtime_error if the file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_ply(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT& uvmap)
{
    static_assert(Dims >= 3, "write_ply requires Dims >= 3");

    const auto [exp_mesh, flat_uvs] = expand_at_seams(mesh, uvmap);

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_ply: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;
    detail::write_ply_header(file, exp_mesh, "", true);
    detail::write_ply_data(file, buf, exp_mesh, flat_uvs);

    if (!file) {
        throw std::runtime_error(
            "write_ply: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// write_ply — Tier 3: positions + UVMap + single texture path
// =============================================================================

/**
 * @brief Write a mesh, UV map, and texture path to an ASCII PLY file
 *
 * Identical to the Tier 2 overload but also emits a
 * @c "comment TextureFile <path>" line immediately after @c "format ascii 1.0"
 * (MeshLab convention).
 *
 * @throws std::runtime_error if the file cannot be opened
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_ply(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT& uvmap,
    const std::filesystem::path& texture_path)
{
    static_assert(Dims >= 3, "write_ply requires Dims >= 3");

    const auto [exp_mesh, flat_uvs] = expand_at_seams(mesh, uvmap);

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_ply: cannot open file: " + path.string());
    }

    std::array<char, 128> buf;
    detail::write_ply_header(file, exp_mesh, texture_path.string(), true);
    detail::write_ply_data(file, buf, exp_mesh, flat_uvs);

    if (!file) {
        throw std::runtime_error(
            "write_ply: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// read_ply — public overloads
// =============================================================================

/**
 * @brief Read a PLY file into a mesh (Tier 1 — positions only)
 *
 * Supports ASCII and binary-little-endian PLY. Parses @c x @c y @c z vertex
 * properties; populates @ref traits::WithNormal and @ref traits::WithColor
 * vertex fields when present in the file and the @c Vertex type supports them.
 * All other properties are skipped.
 *
 * @throws std::runtime_error if the file cannot be opened or is not a PLY file
 */
template <typename T, std::size_t Dims, typename VTraits>
void read_ply(
    const std::filesystem::path& path, Mesh<T, Dims, VTraits>& mesh)
{
    using DummyUV = UVMap<float, 2>;
    DummyUV* no_uvmap = nullptr;
    detail::read_ply_impl(path, mesh, no_uvmap, nullptr);
}

/**
 * @brief Read a PLY file into a mesh and UV map (Tier 2 — positions + UVs)
 *
 * As @ref read_ply(path,mesh) but also parses @c s and @c t per-vertex UV
 * properties into @p uvmap. Texture path comments are ignored.
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void read_ply(
    const std::filesystem::path& path,
    Mesh<T, Dims, VTraits>& mesh,
    UVMapT& uvmap)
{
    detail::read_ply_impl(path, mesh, &uvmap, nullptr);
}

/**
 * @brief Read a PLY file into a mesh, UV map, and texture path list
 *
 * In addition to Tier 1, parses @c s and @c t per-vertex UV properties into
 * @p uvmap. UV pool indices equal vertex indices; wedge mappings are added for
 * all face corners. @c comment @c TextureFile lines from the header are
 * appended to @p texture_paths (empty vector if none present).
 *
 * Note: seam topology is not recovered — the expanded vertex layout written by
 * @ref write_ply is read back as-is.
 *
 * @throws std::runtime_error if the file cannot be opened or is not a PLY file
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void read_ply(
    const std::filesystem::path& path,
    Mesh<T, Dims, VTraits>& mesh,
    UVMapT& uvmap,
    std::vector<std::filesystem::path>& texture_paths)
{
    detail::read_ply_impl(path, mesh, &uvmap, &texture_paths);
}

}  // namespace educelab

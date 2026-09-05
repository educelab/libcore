#pragma once

/** @file */

#include <algorithm>
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
        case Color::Type::U8C1: {
            const auto v = c.value<Color::U8C1>();
            return {v, v, v};
        }
        case Color::Type::U8C3: {
            const auto v = c.value<Color::U8C3>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::U8C4: {
            const auto v = c.value<Color::U8C4>();
            return {v[0], v[1], v[2]};
        }
        case Color::Type::F32C1: {
            const auto v = static_cast<uint8_t>(std::clamp(
                std::lround(c.value<Color::F32C1>() * 255.f), 0L, 255L));
            return {v, v, v};
        }
        case Color::Type::F32C3: {
            const auto v = c.value<Color::F32C3>();
            return {
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[0] * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[1] * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[2] * 255.f), 0L, 255L))};
        }
        case Color::Type::F32C4: {
            const auto v = c.value<Color::F32C4>();
            return {
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[0] * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[1] * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[2] * 255.f), 0L, 255L))};
        }
        case Color::Type::U16C1: {
            const auto v = static_cast<uint8_t>(std::clamp(
                std::lround(c.value<Color::U16C1>() / 65535.f * 255.f), 0L,
                255L));
            return {v, v, v};
        }
        case Color::Type::U16C3: {
            const auto v = c.value<Color::U16C3>();
            return {
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[0] / 65535.f * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[1] / 65535.f * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[2] / 65535.f * 255.f), 0L, 255L))};
        }
        case Color::Type::U16C4: {
            const auto v = c.value<Color::U16C4>();
            return {
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[0] / 65535.f * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[1] / 65535.f * 255.f), 0L, 255L)),
                static_cast<uint8_t>(
                    std::clamp(std::lround(v[2] / 65535.f * 255.f), 0L, 255L))};
        }
        default:
            return {0, 0, 0};
    }
}

// -------------------------------------------------------------------------
// PLY header description
// -------------------------------------------------------------------------

// UV layout note: UVs are stored as a `texcoord` list property on the face
// element. Each face record appends 2*N floats (u0 v0 u1 v1 …) after the
// vertex index list — one UV pair per face corner (per-wedge). This mirrors
// the OBJ approach and avoids the vertex duplication required by the older
// per-vertex `s`/`t` approach. Corners with no UV assignment are written as
// (-1, -1); on read, any texcoord pair of exactly (-1, -1) is treated as
// unmapped and skipped (applies to all files, first and third party).
// Files written with per-vertex `s`/`t` scalar properties are still readable
// for backward compatibility.

/** @brief Semantic role of a PLY property, pre-computed from its name and
 *  element context so the reader inner loops can switch on an integer instead
 *  of comparing strings for every vertex / face. */
enum class PropRole {
    Unknown,
    // Vertex geometry
    X, Y, Z,
    // Vertex normals
    NX, NY, NZ,
    // Vertex colors
    Red, Green, Blue,
    // Legacy per-vertex UV scalars (backward compat)
    S, T,
    // Face list properties
    VertexIndices,
    Texcoord,
};

/** @brief Scalar type of a PLY property, parsed once from the header so inner
 *  loops can switch on an integer instead of comparing strings. */
enum class PLYType {
    Unknown,
    Float,   ///< IEEE 754 single-precision (4 bytes)
    Double,  ///< IEEE 754 double-precision (8 bytes)
    Int,     ///< Signed 32-bit integer
    UInt,    ///< Unsigned 32-bit integer
    Short,   ///< Signed 16-bit integer
    UShort,  ///< Unsigned 16-bit integer
    Char,    ///< Signed 8-bit integer
    UChar,   ///< Unsigned 8-bit integer
};

/** @brief Parse a PLY type token into a @c PLYType enum value
 *
 * Accepts two spellings for each type. The eight names in Greg Turk's original
 * PLY description (`char`, `uchar`, ... `float`, `double`) are the only ones
 * the format defines, and they are what write_ply emits. The sized aliases
 * (`int8`, `uint8`, ... `float32`, `float64`) are not in that description, but
 * they are in wide circulation: vcglib (and therefore MeshLab) has parsed both
 * sets for two decades, and OpenMVS writes the sized ones. Rejecting them
 * means being unable to read files we have to read.
 */
inline auto parse_ply_type(std::string_view t) -> PLYType
{
    if (t == "float" or t == "float32")  return PLYType::Float;
    if (t == "double" or t == "float64") return PLYType::Double;
    if (t == "int" or t == "int32")      return PLYType::Int;
    if (t == "uint" or t == "uint32")    return PLYType::UInt;
    if (t == "short" or t == "int16")    return PLYType::Short;
    if (t == "ushort" or t == "uint16")  return PLYType::UShort;
    if (t == "char" or t == "int8")      return PLYType::Char;
    if (t == "uchar" or t == "uint8")    return PLYType::UChar;
    throw std::runtime_error(
        "read_ply: unrecognized property type '" + std::string(t) + "'");
}

/** @brief Parsed representation of a single PLY property declaration */
struct PLYProp {
    std::string name;                          ///< Property name (e.g., "x", "nx", "red")
    PLYType type{PLYType::Unknown};            ///< Scalar type of the property value
    bool is_list{false};                       ///< True when this is a list property
    PLYType list_count_type{PLYType::Unknown}; ///< Type of the list-length prefix (when @c is_list)
    PropRole role{PropRole::Unknown};          ///< Pre-computed semantic role for dispatch
};

/** @brief A single element block from the PLY header */
struct PLYElement {
    std::string name;               ///< Element name (e.g., "vertex", "face")
    std::size_t count{0};           ///< Number of instances
    std::vector<PLYProp> props;     ///< Ordered list of property declarations
};

/** @brief Parsed PLY file header */
struct PLYHeader {
    /** @brief PLY data-section encoding */
    enum class Format {
        ASCII,     ///< ASCII text encoding
        BinaryLE,  ///< Binary little-endian
        BinaryBE   ///< Binary big-endian
    };
    Format format{Format::ASCII};                      ///< Encoding of the data section
    std::vector<std::filesystem::path> texture_files;  ///< Paths from @c "comment TextureFile" lines
    std::vector<PLYElement> elements;                  ///< Element blocks in declaration order
};

/** @brief Return the byte width of a PLY scalar type */
inline auto ply_type_bytes(PLYType t) -> std::size_t
{
    switch (t) {
        case PLYType::Double: return 8;
        case PLYType::Float:
        case PLYType::Int:
        case PLYType::UInt:   return 4;
        case PLYType::Short:
        case PLYType::UShort: return 2;
        case PLYType::Char:
        case PLYType::UChar:  return 1;
        default:
            throw std::runtime_error("read_ply: unrecognized property type");
    }
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

    std::vector<std::string_view> tokens;
    while (std::getline(file, line)) {
        // TODO: Add line number tracking to exceptions (see MeshIO_OBJ)
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
            // Look for "comment TextureFile <path>". Capture everything from
            // the path's first token to end of line (trimmed) so filenames may
            // contain spaces without a trailing CR / whitespace leaking in.
            if (tokens.size() >= 3 && tokens[1] == "TextureFile") {
                const auto name_pos = std::string_view(line).find(tokens[2]);
                h.texture_files.emplace_back(
                    std::string(trim(std::string_view(line).substr(name_pos))));
            }
        } else if (tokens[0] == "element") {
            if (tokens.size() < 3) {
                throw std::runtime_error(
                    "read_ply: malformed element declaration");
            }
            constexpr std::size_t kMaxElements = 500'000'000;
            const auto n = to_numeric<std::size_t>(tokens[2]);
            if (n > kMaxElements) {
                throw std::runtime_error(
                    "read_ply: element count " + to_string(n) +
                    " exceeds safety limit of " + to_string(kMaxElements));
            }
            h.elements.push_back({std::string(tokens[1]), n, {}});
        } else if (tokens[0] == "property") {
            if (h.elements.empty()) {
                throw std::runtime_error(
                    "read_ply: property declaration outside of element");
            }
            if (tokens.size() < 3) {
                throw std::runtime_error(
                    "read_ply: malformed property declaration");
            }
            PLYProp p;
            if (tokens[1] == "list" && tokens.size() >= 5) {
                p.is_list = true;
                p.list_count_type = parse_ply_type(tokens[2]);
                p.type = parse_ply_type(tokens[3]);
                p.name = std::string(tokens[4]);
            } else {
                p.name = std::string(tokens[2]);
                p.type = parse_ply_type(tokens[1]);
            }
            // Assign semantic role once so inner read loops switch on an
            // integer rather than comparing strings per vertex / face.
            const auto& ename = h.elements.back().name;
            if (ename == "vertex") {
                if      (p.name == "x")     p.role = PropRole::X;
                else if (p.name == "y")     p.role = PropRole::Y;
                else if (p.name == "z")     p.role = PropRole::Z;
                else if (p.name == "nx")    p.role = PropRole::NX;
                else if (p.name == "ny")    p.role = PropRole::NY;
                else if (p.name == "nz")    p.role = PropRole::NZ;
                else if (p.name == "red")   p.role = PropRole::Red;
                else if (p.name == "green") p.role = PropRole::Green;
                else if (p.name == "blue")  p.role = PropRole::Blue;
                else if (p.name == "s")     p.role = PropRole::S;
                else if (p.name == "t")     p.role = PropRole::T;
            } else if (ename == "face") {
                if      (p.name == "vertex_indices") p.role = PropRole::VertexIndices;
                else if (p.name == "texcoord")       p.role = PropRole::Texcoord;
            }
            h.elements.back().props.push_back(std::move(p));
        }
    }
    return h;
}

// -------------------------------------------------------------------------
// Binary property reading (little-endian)
// -------------------------------------------------------------------------

/** @brief Read a single binary little-endian PLY scalar property from @p f */
template <typename DestT>
auto read_ply_binary_prop(std::istream& f, PLYType type) -> DestT
{
    const auto err = []() {
        throw std::runtime_error("read_ply: unexpected end of binary data");
    };
    switch (type) {
        case PLYType::Float: {
            float v{};
            f.read(reinterpret_cast<char*>(&v), 4);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::Double: {
            double v{};
            f.read(reinterpret_cast<char*>(&v), 8);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::Int: {
            int32_t v{};
            f.read(reinterpret_cast<char*>(&v), 4);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::UInt: {
            uint32_t v{};
            f.read(reinterpret_cast<char*>(&v), 4);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::Short: {
            int16_t v{};
            f.read(reinterpret_cast<char*>(&v), 2);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::UShort: {
            uint16_t v{};
            f.read(reinterpret_cast<char*>(&v), 2);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::Char: {
            int8_t v{};
            f.read(reinterpret_cast<char*>(&v), 1);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        case PLYType::UChar: {
            uint8_t v{};
            f.read(reinterpret_cast<char*>(&v), 1);
            if (!f) err();
            return static_cast<DestT>(v);
        }
        default:
            // The header parser rejects unrecognized types before we reach
            // here, but guard defensively so the function is correct in
            // isolation.
            throw std::runtime_error("read_ply: unrecognized property type");
    }
}

/** @brief Extract a typed value from a raw byte buffer using memcpy.
 *
 *  Companion to read_ply_binary_prop for use when the full vertex record has
 *  been read in one istream::read call. Each field is extracted by its
 *  pre-computed byte offset within the buffer rather than via individual
 *  istream::read calls.
 */
template <typename DestT>
auto read_ply_prop_from_buf(const char* buf, PLYType type) -> DestT
{
    switch (type) {
        case PLYType::Float: {
            float v;
            std::memcpy(&v, buf, 4);
            return static_cast<DestT>(v);
        }
        case PLYType::Double: {
            double v;
            std::memcpy(&v, buf, 8);
            return static_cast<DestT>(v);
        }
        case PLYType::Int: {
            int32_t v;
            std::memcpy(&v, buf, 4);
            return static_cast<DestT>(v);
        }
        case PLYType::UInt: {
            uint32_t v;
            std::memcpy(&v, buf, 4);
            return static_cast<DestT>(v);
        }
        case PLYType::Short: {
            int16_t v;
            std::memcpy(&v, buf, 2);
            return static_cast<DestT>(v);
        }
        case PLYType::UShort: {
            uint16_t v;
            std::memcpy(&v, buf, 2);
            return static_cast<DestT>(v);
        }
        case PLYType::Char: {
            int8_t v;
            std::memcpy(&v, buf, 1);
            return static_cast<DestT>(v);
        }
        case PLYType::UChar: {
            uint8_t v;
            std::memcpy(&v, buf, 1);
            return static_cast<DestT>(v);
        }
        default:
            throw std::runtime_error("read_ply: unrecognized property type");
    }
}

// -------------------------------------------------------------------------
// Face record parsing helpers
// -------------------------------------------------------------------------

// Bounds on a list property's element count. Legitimate values are small.
// Unknown list properties are skipped rather than interpreted, but the count
// still governs how many bytes are advanced, so it needs a bound there too:
// PLY permits a signed count type, and a negative count converted to an
// unsigned byte total wraps to a seek that skips nothing, leaving the reader
// misaligned inside the element it meant to step over.

/** @brief Largest @c vertex_indices count @ref read_ply will accept */
constexpr std::size_t kMaxFaceVertices = 256;

/** @brief Largest count @ref read_ply will accept for any other list */
constexpr std::size_t kMaxFaceListLength = 1024;

/** @brief Parse one binary face record from @p file.
 *
 *  Populates @p face with vertex indices and, when @p load_texcoords is
 *  true, @p texcoords with raw float values. Both containers are cleared
 *  before filling. Skips any list or scalar face properties that are not
 *  vertex_indices or texcoord.
 */
inline void read_ply_face_binary(
    std::istream& file,
    const PLYElement& elem,
    std::size_t n_vertices,
    bool load_texcoords,
    std::vector<std::size_t>& face,
    std::vector<float>& texcoords)
{
    face.clear();
    texcoords.clear();
    for (const auto& prop : elem.props) {
        if (not prop.is_list) {
            file.ignore(
                static_cast<std::streamsize>(ply_type_bytes(prop.type)));
            continue;
        }
        const auto count =
            read_ply_binary_prop<std::size_t>(file, prop.list_count_type);
        switch (prop.role) {
            case PropRole::VertexIndices:
                if (count > kMaxFaceVertices) {
                    throw std::runtime_error(
                        "read_ply: face vertex count " + to_string(count) +
                        " exceeds maximum of " +
                        to_string(kMaxFaceVertices));
                }
                face.reserve(count);
                for (std::size_t k = 0; k < count; ++k) {
                    const auto idx =
                        read_ply_binary_prop<std::size_t>(file, prop.type);
                    if (idx >= n_vertices) {
                        throw std::runtime_error(
                            "read_ply: face vertex index " +
                            to_string(idx) +
                            " out of range (n_vertices=" +
                            to_string(n_vertices) + ")");
                    }
                    face.push_back(idx);
                }
                break;
            case PropRole::Texcoord:
                if (count > kMaxFaceListLength) {
                    throw std::runtime_error(
                        "read_ply: face texcoord count " +
                        to_string(count) + " exceeds maximum of " +
                        to_string(kMaxFaceListLength));
                }
                if (load_texcoords) {
                    texcoords.resize(count);
                    for (std::size_t k = 0; k < count; ++k) {
                        texcoords[k] =
                            read_ply_binary_prop<float>(file, prop.type);
                    }
                } else {
                    file.ignore(static_cast<std::streamsize>(
                        count * ply_type_bytes(prop.type)));
                }
                break;
            default:
                if (count > kMaxFaceListLength) {
                    throw std::runtime_error(
                        "read_ply: face list property count " +
                        to_string(count) + " exceeds maximum of " +
                        to_string(kMaxFaceListLength));
                }
                file.ignore(static_cast<std::streamsize>(
                    count * ply_type_bytes(prop.type)));
                break;
        }
    }
}

/** @brief Parse one ASCII face record from @p tokens.
 *
 *  @p tokens must already be split from the data line. Populates @p face
 *  with vertex indices and, when @p load_texcoords is true, @p texcoords
 *  with raw float values. Both containers are cleared before filling.
 *  Walks tokens in property-declaration order, skipping unknown properties.
 */
inline void read_ply_face_ascii(
    const std::vector<std::string_view>& tokens,
    const PLYElement& elem,
    std::size_t n_vertices,
    bool load_texcoords,
    std::vector<std::size_t>& face,
    std::vector<float>& texcoords)
{
    face.clear();
    texcoords.clear();
    std::size_t ti = 0;
    for (const auto& prop : elem.props) {
        if (ti >= tokens.size())
            break;
        if (not prop.is_list) {
            ++ti;
            continue;
        }
        const auto count = to_numeric<std::size_t>(tokens[ti++]);
        switch (prop.role) {
            case PropRole::VertexIndices:
                if (count > kMaxFaceVertices) {
                    throw std::runtime_error(
                        "read_ply: face vertex count " + to_string(count) +
                        " exceeds maximum of " +
                        to_string(kMaxFaceVertices));
                }
                face.reserve(count);
                for (std::size_t k = 0;
                     k < count && ti < tokens.size();
                     ++k, ++ti) {
                    const auto idx = to_numeric<std::size_t>(tokens[ti]);
                    if (idx >= n_vertices) {
                        throw std::runtime_error(
                            "read_ply: face vertex index " +
                            to_string(idx) +
                            " out of range (n_vertices=" +
                            to_string(n_vertices) + ")");
                    }
                    face.push_back(idx);
                }
                break;
            case PropRole::Texcoord:
                if (count > kMaxFaceListLength) {
                    throw std::runtime_error(
                        "read_ply: face texcoord count " +
                        to_string(count) + " exceeds maximum of " +
                        to_string(kMaxFaceListLength));
                }
                if (load_texcoords) {
                    texcoords.resize(count);
                    for (std::size_t k = 0;
                         k < count && ti < tokens.size();
                         ++k, ++ti) {
                        texcoords[k] = to_numeric<float>(tokens[ti]);
                    }
                } else {
                    ti += count;
                }
                break;
            default:
                if (count > kMaxFaceListLength) {
                    throw std::runtime_error(
                        "read_ply: face list property count " +
                        to_string(count) + " exceeds maximum of " +
                        to_string(kMaxFaceListLength));
                }
                ti += count;
                break;
        }
    }
}

// -------------------------------------------------------------------------
// Core PLY reader
// -------------------------------------------------------------------------

/** @brief Internal PLY reader shared by all public @ref read_ply overloads */
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

    // Locate the vertex and face elements so we can inspect their properties
    // before the main read loop.
    const PLYElement* vert_elem = nullptr;
    const PLYElement* face_elem = nullptr;
    for (const auto& elem : hdr.elements) {
        if (elem.name == "vertex")
            vert_elem = &elem;
        else if (elem.name == "face")
            face_elem = &elem;
    }

    // Identify which optional vertex attributes the file carries.
    // Roles were pre-computed by parse_ply_header, so no string comparisons here.
    bool has_nx{false}, has_ny{false}, has_nz{false};
    bool has_r{false}, has_g{false}, has_b{false};
    bool has_s{false}, has_t{false};
    // Declared PLY type of the red property (taken as canonical for r/g/b).
    // Drives which Color variant is stored — uchar/ushort/float preserve the
    // file's native representation rather than forcing a lossy conversion.
    PLYType color_type{PLYType::Unknown};
    if (vert_elem) {
        for (const auto& p : vert_elem->props) {
            switch (p.role) {
                case PropRole::NX:    has_nx = true; break;
                case PropRole::NY:    has_ny = true; break;
                case PropRole::NZ:    has_nz = true; break;
                case PropRole::Red:
                    has_r = true;
                    color_type = p.type;
                    break;
                case PropRole::Green: has_g  = true; break;
                case PropRole::Blue:  has_b  = true; break;
                case PropRole::S:     has_s  = true; break;
                case PropRole::T:     has_t  = true; break;
                default: break;
            }
        }
    }

    const std::size_t n_vertices = vert_elem ? vert_elem->count : 0;

    // Detect per-wedge texcoord list property on the face element.
    // When present, UVs are read from the face texcoord list rather than from
    // per-vertex s/t scalars.
    bool has_texcoord{false};
    if (face_elem) {
        for (const auto& p : face_elem->props) {
            if (p.is_list && p.role == PropRole::Texcoord) {
                has_texcoord = true;
                break;
            }
        }
    }

    // Precompute the backward-compat condition once; reused in vertex and face
    // loops to avoid repeating the same four-term expression.
    const bool legacy_st_uvs =
        uvmap != nullptr && has_s && has_t && !has_texcoord;

    // Helpers to skip unknown-element data without interpreting it.
    auto skip_binary_prop = [&](const PLYProp& prop) {
        if (not prop.is_list) {
            file.ignore(
                static_cast<std::streamsize>(ply_type_bytes(prop.type)));
            return;
        }
        const auto count =
            read_ply_binary_prop<std::size_t>(file, prop.list_count_type);
        // Bound before multiplying. A negative count from a signed count type
        // arrives here as a huge unsigned value, and the byte total would wrap
        // to a negative seek that skips nothing at all.
        if (count > kMaxFaceListLength) {
            throw std::runtime_error(
                "read_ply: list property count " + to_string(count) +
                " exceeds maximum of " + to_string(kMaxFaceListLength));
        }
        file.ignore(
            static_cast<std::streamsize>(count * ply_type_bytes(prop.type)));
    };

    auto skip_ascii_line = [&]() {
        std::string skip_line;
        while (std::getline(file, skip_line)) {
            // Trim trailing whitespace (incl. '\r' on Windows) then skip
            // '#'-comment lines; break on the first real data line.
            const auto sv = trim_right(skip_line);
            if (!sv.empty() && sv.front() != '#')
                break;
        }
    };

    // Iterate all elements in declaration order, processing vertex and face
    // blocks and skipping any others (e.g. edge, tristrips, or
    // application-defined elements) to stay positioned in the data stream.
    std::string line;
    std::vector<std::string_view> tokens;
    for (const auto& elem : hdr.elements) {
        if (elem.name == "vertex") {
            // Neither vertex path can interpret a list property: the binary
            // reader sizes each record by summing its properties' scalar
            // widths, and the ASCII reader indexes tokens by property
            // position. A list occupies a count plus N values, so both would
            // read the first vertex correctly and every later one from the
            // wrong offset. Refuse the file rather than return garbage.
            for (const auto& p : elem.props) {
                if (p.is_list) {
                    throw std::runtime_error(
                        "read_ply: list property '" + p.name +
                        "' on the vertex element is not supported");
                }
            }

            // Pre-compute binary vertex record layout so the inner loop makes
            // one file.read() per vertex (O(vertices)) instead of one read
            // per property per vertex (O(properties × vertices)).
            constexpr std::size_t kMaxVertBufBytes = 256;
            std::size_t vert_rec_size = 0;
            std::vector<std::size_t> vert_offsets;
            if (binary) {
                vert_offsets.reserve(elem.props.size());
                for (const auto& p : elem.props) {
                    vert_offsets.push_back(vert_rec_size);
                    vert_rec_size += ply_type_bytes(p.type);
                }
                if (vert_rec_size > kMaxVertBufBytes) {
                    throw std::runtime_error(
                        "read_ply: vertex record size " +
                        to_string(vert_rec_size) +
                        " bytes exceeds buffer limit of " +
                        to_string(kMaxVertBufBytes));
                }
            }
            std::array<char, kMaxVertBufBytes> vbuf{};

            for (std::size_t vi = 0; vi < elem.count; ++vi) {
                T x{}, y{}, z{};
                T nx{}, ny{}, nz{};
                float r{}, g{}, b{};
                float s{}, t{};

                if (binary) {
                    file.read(
                        vbuf.data(),
                        static_cast<std::streamsize>(vert_rec_size));
                    if (!file) {
                        throw std::runtime_error(
                            "read_ply: unexpected end of binary vertex data");
                    }
                    for (std::size_t pi = 0; pi < elem.props.size(); ++pi) {
                        const auto& prop = elem.props[pi];
                        const char* pb = vbuf.data() + vert_offsets[pi];
                        switch (prop.role) {
                            case PropRole::X:
                                x = read_ply_prop_from_buf<T>(pb, prop.type); break;
                            case PropRole::Y:
                                y = read_ply_prop_from_buf<T>(pb, prop.type); break;
                            case PropRole::Z:
                                z = read_ply_prop_from_buf<T>(pb, prop.type); break;
                            case PropRole::NX:
                                nx = read_ply_prop_from_buf<T>(pb, prop.type); break;
                            case PropRole::NY:
                                ny = read_ply_prop_from_buf<T>(pb, prop.type); break;
                            case PropRole::NZ:
                                nz = read_ply_prop_from_buf<T>(pb, prop.type); break;
                            case PropRole::Red:
                                r = read_ply_prop_from_buf<float>(pb, prop.type); break;
                            case PropRole::Green:
                                g = read_ply_prop_from_buf<float>(pb, prop.type); break;
                            case PropRole::Blue:
                                b = read_ply_prop_from_buf<float>(pb, prop.type); break;
                            case PropRole::S:
                                s = read_ply_prop_from_buf<float>(pb, prop.type); break;
                            case PropRole::T:
                                t = read_ply_prop_from_buf<float>(pb, prop.type); break;
                            default:
                                break;  // unknown: in buffer, role ignored
                        }
                    }
                } else {
                    // ASCII: skip '#'-comment lines.
                    std::string_view sv;
                    while (std::getline(file, line)) {
                        sv = trim_right(line);
                        if (!sv.empty() && sv.front() != '#')
                            break;
                    }
                    split(sv, tokens);
                    for (std::size_t pi = 0;
                         pi < elem.props.size() && pi < tokens.size(); ++pi) {
                        switch (elem.props[pi].role) {
                            case PropRole::X:
                                x = to_numeric<T>(tokens[pi]); break;
                            case PropRole::Y:
                                y = to_numeric<T>(tokens[pi]); break;
                            case PropRole::Z:
                                z = to_numeric<T>(tokens[pi]); break;
                            case PropRole::NX:
                                nx = to_numeric<T>(tokens[pi]); break;
                            case PropRole::NY:
                                ny = to_numeric<T>(tokens[pi]); break;
                            case PropRole::NZ:
                                nz = to_numeric<T>(tokens[pi]); break;
                            case PropRole::Red:
                                r = to_numeric<float>(tokens[pi]); break;
                            case PropRole::Green:
                                g = to_numeric<float>(tokens[pi]); break;
                            case PropRole::Blue:
                                b = to_numeric<float>(tokens[pi]); break;
                            case PropRole::S:
                                s = to_numeric<float>(tokens[pi]); break;
                            case PropRole::T:
                                t = to_numeric<float>(tokens[pi]); break;
                            default: break;  // unknown: token already indexed by pi, skip
                        }
                    }
                }

                const auto new_vi = mesh.insert_vertex(x, y, z);

                if constexpr (traits::has_normal<Vertex>::value) {
                    if (has_nx && has_ny && has_nz) {
                        Vec<T, Dims> n{nx, ny, nz};
                        mesh.vertex(new_vi).normal = n;
                    }
                }
                if constexpr (traits::has_color<Vertex>::value) {
                    if (has_r && has_g && has_b) {
                        // Store the file's native representation. Values are
                        // read through a float intermediate, which is lossless
                        // for uchar (0-255) and ushort (0-65535) since both
                        // ranges fit exactly in float.
                        if (color_type == PLYType::UChar) {
                            mesh.vertex(new_vi).color = Color::U8C3{
                                static_cast<uint8_t>(r),
                                static_cast<uint8_t>(g),
                                static_cast<uint8_t>(b)};
                        } else if (color_type == PLYType::UShort) {
                            mesh.vertex(new_vi).color = Color::U16C3{
                                static_cast<uint16_t>(r),
                                static_cast<uint16_t>(g),
                                static_cast<uint16_t>(b)};
                        } else {
                            // float / double / int / uint — store as F32C3
                            mesh.vertex(new_vi).color = Color::F32C3{r, g, b};
                        }
                    }
                }
                if (legacy_st_uvs) {
                    // Pool index equals vertex index by construction.
                    (void)uvmap->insert(s, t);
                }
            }

        } else if (elem.name == "face") {
            // Pre-size face→UV index to avoid per-face resizes inside map().
            if (uvmap != nullptr)
                uvmap->reserve_faces(elem.count);

            // Pre-compute once: texcoords are only loaded when both a uvmap
            // is provided and the face element has a texcoord list property.
            const bool load_texcoords = uvmap != nullptr && has_texcoord;

            std::vector<float> texcoords;          // reused across face iterations
            std::vector<std::size_t> face_indices; // reused buffer for helpers
            std::string fline;                     // reused for ASCII reads
            for (std::size_t fi = 0; fi < elem.count; ++fi) {
                if (binary) {
                    read_ply_face_binary(
                        file, elem, n_vertices, load_texcoords,
                        face_indices, texcoords);
                } else {
                    std::string_view sv;
                    while (std::getline(file, fline)) {
                        sv = trim_right(fline);
                        if (!sv.empty() && sv.front() != '#')
                            break;
                    }
                    split(sv, tokens);
                    if (tokens.empty())
                        continue;
                    read_ply_face_ascii(
                        tokens, elem, n_vertices, load_texcoords,
                        face_indices, texcoords);
                }

                typename Mesh<T, Dims, VTraits>::Face face(
                    face_indices.begin(), face_indices.end());

                const auto new_fi = mesh.insert_face(face);

                // Per-wedge UVs from texcoord list.
                // Any pair that is exactly (-1, -1) is the "no UV" sentinel
                // and is skipped (applies to all files, first and third party).
                if (uvmap != nullptr && has_texcoord &&
                    texcoords.size() == 2 * face.size()) {
                    for (std::size_t ci = 0; ci < face.size(); ++ci) {
                        const float u = texcoords[2 * ci];
                        const float v = texcoords[2 * ci + 1];
                        if (u == -1.f && v == -1.f)
                            continue;
                        const auto uv_idx = uvmap->insert(u, v);
                        uvmap->map(new_fi, ci, uv_idx);
                    }
                }

                // Backward compat: map per-vertex s/t UVs.
                // Pool entries were inserted in vertex order above, so
                // pool index == vertex index.
                if (legacy_st_uvs) {
                    for (std::size_t ci = 0; ci < face.size(); ++ci) {
                        uvmap->map(new_fi, ci, face[ci]);
                    }
                }
            }

        } else {
            // Unknown element — skip all records to stay positioned correctly
            if (binary) {
                for (std::size_t i = 0; i < elem.count; ++i) {
                    for (const auto& prop : elem.props) {
                        skip_binary_prop(prop);
                    }
                }
            } else {
                for (std::size_t i = 0; i < elem.count; ++i) {
                    skip_ascii_line();
                }
            }
        }
    }
}

/**
 * @brief Write the PLY ASCII header to @p file
 *
 * Shared by all write_ply tiers. The @p texture_comment parameter may be
 * empty (no @c comment TextureFile line) or contain a path string.
 * The @p has_uvs flag controls whether a @c texcoord list property is
 * declared on the face element. The @p has_normals flag (from
 * @ref has_any_normal) controls whether @c nx/ny/nz are declared; the
 * @p has_colors flag (from @ref has_any_color) controls whether
 * @c red/green/blue are declared.
 */
template <typename T, std::size_t Dims, typename VTraits>
void write_ply_header(
    std::ostream& file,
    const Mesh<T, Dims, VTraits>& mesh,
    const std::string& texture_comment,
    bool has_uvs,
    [[maybe_unused]] bool has_normals,
    [[maybe_unused]] bool has_colors)
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
        if (has_normals) {
            file << "property float nx\n"
                 << "property float ny\n"
                 << "property float nz\n";
        }
    }
    if constexpr (traits::has_color<Vertex>::value) {
        if (has_colors) {
            file << "property uchar red\n"
                 << "property uchar green\n"
                 << "property uchar blue\n";
        }
    }

    file << "element face " << mesh.num_faces() << '\n'
         << "property list uchar int vertex_indices\n";

    if (has_uvs) {
        file << "property list uchar float texcoord\n";
    }

    file << "end_header\n";
}

/**
 * @brief Write PLY ASCII vertex and face data to @p file
 *
 * @p uvmap may be @c nullptr (no UV output). When non-null, each face record
 * is followed by a @c texcoord list of 2*N floats. Unmapped corners are
 * written as @c -1,-1 (sentinel for "no UV assignment"). @p has_normals and
 * @p has_colors must match the values passed to @ref write_ply_header so the
 * data matches the declared properties.
 */
template <typename T, std::size_t Dims, typename VTraits, typename UVMapT>
void write_ply_data(
    std::ostream& file,
    std::array<char, 128>& buf,
    const Mesh<T, Dims, VTraits>& mesh,
    const UVMapT* uvmap,
    [[maybe_unused]] bool has_normals,
    [[maybe_unused]] bool has_colors)
{
    using Vertex = typename Mesh<T, Dims, VTraits>::Vertex;

    for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
        const auto& v = mesh.vertex(vi);
        file << to_string_view(buf, v[0]) << ' '
             << to_string_view(buf, v[1]) << ' '
             << to_string_view(buf, v[2]);
        if constexpr (traits::has_normal<Vertex>::value) {
            if (has_normals) {
                // PLY's fixed-property element forces a value for every
                // vertex; gaps in a partially-normalled mesh fall back to zero.
                const auto n = v.normal.value_or(Vec<T, Dims>{});
                file << ' ' << to_string_view(buf, n[0])
                     << ' ' << to_string_view(buf, n[1])
                     << ' ' << to_string_view(buf, n[2]);
            }
        }
        if constexpr (traits::has_color<Vertex>::value) {
            if (has_colors) {
                // PLY's fixed-property element forces a value for every
                // vertex; gaps in a partially-colored mesh fall back to black.
                const auto [r, g, b] = detail::color_to_u8c3(v.color);
                file << ' ' << to_string_view(buf, r)
                     << ' ' << to_string_view(buf, g)
                     << ' ' << to_string_view(buf, b);
            }
        }
        file << '\n';
    }

    for (std::size_t fi = 0; fi < mesh.num_faces(); ++fi) {
        const auto& face = mesh.face(fi);
        file << face.size();
        for (const auto vi : face) {
            file << ' ' << to_string_view(buf, vi);
        }
        if (uvmap != nullptr) {
            // texcoord list: 2*N floats, one UV pair per corner.
            // Unmapped corners use the (-1, -1) sentinel.
            file << ' ' << to_string_view(buf, 2 * face.size());
            for (std::size_t ci = 0; ci < face.size(); ++ci) {
                float u{-1.f}, v{-1.f};
                if (uvmap->has(fi, ci)) {
                    const auto& uv = uvmap->at(uvmap->get(fi, ci));
                    u = static_cast<float>(uv[0]);
                    v = static_cast<float>(uv[1]);
                }
                file << ' ' << to_string_view(buf, u) << ' '
                     << to_string_view(buf, v);
            }
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
 * @ref traits::WithNormal @em and at least one vertex has a normal set, also
 * emits @c nx @c ny @c nz properties (a normal-less mesh declares none). If
 * @c Vertex carries @ref traits::WithColor @em and at least one vertex has a
 * color set, also emits @c red @c green @c blue properties (@c uchar, 0–255;
 * a color-less mesh declares none).
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

    std::array<char, 128> buf{};
    const bool has_normals = has_any_normal(mesh);
    const bool has_colors = has_any_color(mesh);
    detail::write_ply_header(file, mesh, "", false, has_normals, has_colors);
    detail::write_ply_data(
        file, buf, mesh, static_cast<const UVMap<float, 2>*>(nullptr),
        has_normals, has_colors);

    // Close before checking. The stream may still hold buffered data at this
    // point; the final flush happens when `file` is destroyed, and a failure
    // there would be swallowed, so write_ply would return normally on an
    // incomplete file. close() performs that flush and records its failure.
    file.close();
    if (!file) {
        throw std::runtime_error(
            "write_ply: I/O error while writing file: " + path.string());
    }
}

// =============================================================================
// write_ply — Tier 2: positions + UVMap (per-wedge texcoord)
// =============================================================================

/**
 * @brief Write a mesh and UV map to an ASCII PLY file
 *
 * Writes per-wedge UV coordinates as a @c texcoord list property on the face
 * element. No vertex duplication is performed. Adds
 * @c "property list uchar float texcoord" to the face element header.
 * Corners with no UV assignment are written as @c -1,-1.
 * No @c comment @c TextureFile line is written.
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

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_ply: cannot open file: " + path.string());
    }

    std::array<char, 128> buf{};
    const bool has_normals = has_any_normal(mesh);
    const bool has_colors = has_any_color(mesh);
    detail::write_ply_header(file, mesh, "", true, has_normals, has_colors);
    detail::write_ply_data(file, buf, mesh, &uvmap, has_normals, has_colors);

    // Close before checking. The stream may still hold buffered data at this
    // point; the final flush happens when `file` is destroyed, and a failure
    // there would be swallowed, so write_ply would return normally on an
    // incomplete file. close() performs that flush and records its failure.
    file.close();
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
 * @note PLY write supports only a **single** texture/chart: there is no
 *       @c std::vector<std::filesystem::path> overload as there is for
 *       @c write_obj, because multi-texture PLY has no well-supported ecosystem
 *       standard outside MeshLab. Multi-chart meshes should be written to OBJ.
 *       (@c read_ply will still recover multiple @c "comment TextureFile" lines
 *       into its @c texture_paths out-parameter when reading such files.)
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

    std::ofstream file(path);
    if (!file) {
        throw std::runtime_error(
            "write_ply: cannot open file: " + path.string());
    }

    std::array<char, 128> buf{};
    const bool has_normals = has_any_normal(mesh);
    const bool has_colors = has_any_color(mesh);
    detail::write_ply_header(
        file, mesh, texture_path.string(), true, has_normals, has_colors);
    detail::write_ply_data(file, buf, mesh, &uvmap, has_normals, has_colors);

    // Close before checking. The stream may still hold buffered data at this
    // point; the final flush happens when `file` is destroyed, and a failure
    // there would be swallowed, so write_ply would return normally on an
    // incomplete file. close() performs that flush and records its failure.
    file.close();
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
 * As the two-argument @ref read_ply overload but also parses per-wedge UV coordinates from a
 * @c "property list uchar float texcoord" face property into @p uvmap. Falls
 * back to legacy per-vertex @c s / @c t scalar properties when texcoord is
 * absent. Texture path comments are ignored.
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
 * In addition to Tier 1, parses per-wedge UV coordinates from a
 * @c "property list uchar float texcoord" face property into @p uvmap. Falls
 * back to legacy per-vertex @c s / @c t scalar properties when texcoord is
 * absent. @c comment @c TextureFile lines from the PLY header are appended to
 * @p texture_paths (empty vector if none present).
 *
 * @note Multiple @c "comment TextureFile" lines are recovered here for
 *       compatibility with multi-chart files produced by other tools, but
 *       @c write_ply only ever emits a single texture path — see the
 *       single-texture @c write_ply overload.
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

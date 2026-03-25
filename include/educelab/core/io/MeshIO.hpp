#pragma once

/** @file */

#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>

#include "educelab/core/io/MeshIO_OBJ.hpp"
#include "educelab/core/io/MeshIO_PLY.hpp"

namespace educelab
{

/**
 * @brief Read a mesh from a file, dispatching by extension
 *
 * Supported extensions: `.obj`, `.ply`.  Throws `std::runtime_error` for
 * any other extension.
 *
 * @tparam T     Mesh numeric type
 * @tparam Dims  Mesh dimensionality
 * @tparam VT    Vertex traits type
 */
template <typename T, std::size_t Dims, typename VT>
void read_mesh(
    const std::filesystem::path& path,
    Mesh<T, Dims, VT>& mesh)
{
    const auto ext = path.extension().string();
    if (ext == ".obj")
        read_obj(path, mesh);
    else if (ext == ".ply")
        read_ply(path, mesh);
    else
        throw std::runtime_error("read_mesh: unsupported extension '" + ext + "'");
}

/**
 * @brief Write a mesh to a file, dispatching by extension
 *
 * Supported extensions: `.obj`, `.ply`.  Throws `std::runtime_error` for
 * any other extension.
 */
template <typename T, std::size_t Dims, typename VT>
void write_mesh(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VT>& mesh)
{
    const auto ext = path.extension().string();
    if (ext == ".obj")
        write_obj(path, mesh);
    else if (ext == ".ply")
        write_ply(path, mesh);
    else
        throw std::runtime_error("write_mesh: unsupported extension '" + ext + "'");
}

/**
 * @brief Read a mesh and UV map from a file, dispatching by extension
 */
template <typename T, std::size_t Dims, typename VT, typename UVMapT>
void read_mesh(
    const std::filesystem::path& path,
    Mesh<T, Dims, VT>& mesh,
    UVMapT& uvmap)
{
    const auto ext = path.extension().string();
    if (ext == ".obj") {
        read_obj(path, mesh, uvmap);
    } else if (ext == ".ply") {
        std::vector<std::filesystem::path> unused;
        read_ply(path, mesh, uvmap, unused);
    } else {
        throw std::runtime_error("read_mesh: unsupported extension '" + ext + "'");
    }
}

/**
 * @brief Write a mesh and UV map to a file, dispatching by extension
 */
template <typename T, std::size_t Dims, typename VT, typename UVMapT>
void write_mesh(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VT>& mesh,
    const UVMapT& uvmap)
{
    const auto ext = path.extension().string();
    if (ext == ".obj")
        write_obj(path, mesh, uvmap);
    else if (ext == ".ply")
        write_ply(path, mesh, uvmap);
    else
        throw std::runtime_error("write_mesh: unsupported extension '" + ext + "'");
}

/**
 * @brief Read a mesh, UV map, and texture paths from a file, dispatching by
 *        extension
 *
 * @p texture_paths is populated by `map_Kd` entries (OBJ) or
 * `comment TextureFile` lines (PLY).
 */
template <typename T, std::size_t Dims, typename VT, typename UVMapT>
void read_mesh(
    const std::filesystem::path& path,
    Mesh<T, Dims, VT>& mesh,
    UVMapT& uvmap,
    std::vector<std::filesystem::path>& texture_paths)
{
    const auto ext = path.extension().string();
    if (ext == ".obj")
        read_obj(path, mesh, uvmap, texture_paths);
    else if (ext == ".ply")
        read_ply(path, mesh, uvmap, texture_paths);
    else
        throw std::runtime_error("read_mesh: unsupported extension '" + ext + "'");
}

/**
 * @brief Write a mesh, UV map, and single texture path to a file, dispatching
 *        by extension
 *
 * OBJ: emits a `.mtl` with one `map_Kd` entry.\n
 * PLY: emits a `comment TextureFile` line in the header.
 */
template <typename T, std::size_t Dims, typename VT, typename UVMapT>
void write_mesh(
    const std::filesystem::path& path,
    const Mesh<T, Dims, VT>& mesh,
    const UVMapT& uvmap,
    const std::filesystem::path& texture_path)
{
    const auto ext = path.extension().string();
    if (ext == ".obj")
        write_obj(path, mesh, uvmap, texture_path);
    else if (ext == ".ply")
        write_ply(path, mesh, uvmap, texture_path);
    else
        throw std::runtime_error("write_mesh: unsupported extension '" + ext + "'");
}

}  // namespace educelab

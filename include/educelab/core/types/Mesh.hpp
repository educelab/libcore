#pragma once

/** @file */

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <variant>
#include <vector>

#include "educelab/core/types/Color.hpp"
#include "educelab/core/types/Vec.hpp"

namespace educelab
{

namespace traits
{
/**
 * @brief Opt-in vertex normal mixin
 *
 * Compose into a custom traits struct via multiple inheritance to add
 * per-vertex normal storage. Detected at compile time by I/O functions
 * via @c if @c constexpr and the C++17 detection idiom.
 *
 * @tparam T Numeric type of the normal vector
 * @tparam Dims Number of dimensions of the normal vector
 */
template <
    typename T,
    std::size_t Dims,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
struct WithNormal {
    /** @brief Vertex normal */
    std::optional<Vec<T, Dims>> normal;
};

/**
 * @brief Opt-in vertex color mixin
 *
 * Compose into a custom traits struct via multiple inheritance to add
 * per-vertex color storage. Detected at compile time by I/O functions
 * via @c if @c constexpr and the C++17 detection idiom.
 */
struct WithColor {
    /** @brief Vertex color */
    Color color;
};

/**
 * @brief Default traits for Mesh vertices
 *
 * Empty by default. To add normals, colors, or other per-vertex data,
 * compose @ref WithNormal, @ref WithColor, or a custom mixin struct via
 * multiple inheritance:
 * @code
 * struct MyTraits : traits::WithNormal<float,3>, traits::WithColor {};
 * using MyMesh = Mesh<float, 3, MyTraits>;
 * @endcode
 *
 * @tparam T Mesh numeric type
 * @tparam Dims Mesh dimensions
 */
template <
    typename T,
    std::size_t Dims,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
struct DefaultVertexTraits {
};
}  // namespace traits

/**
 * @brief Basic mesh class
 *
 * @tparam T Numeric type to use for coordinate system
 * @tparam Dims Number of dimensions in the coordinate system
 * @tparam VertexTraits Additional vertex traits
 */
template <
    typename T,
    std::size_t Dims,
    typename VertexTraits = traits::DefaultVertexTraits<T, Dims>,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
class Mesh
{
public:
    /** Pointer type */
    using Pointer = std::shared_ptr<Mesh>;

    /** @brief %Vertex type */
    struct Vertex : public Vec<T, Dims>, public VertexTraits {
        /** @brief Default constructor */
        Vertex() = default;

        /**
         * @brief Construct with element values
         *
         * The number of arguments provided must match Dims.
         */
        template <typename... Args>
        explicit Vertex(Args... args) : Vec<T, Dims>{args...}
        {
        }

        /** Inherit assignment operators */
        using Vec<T, Dims>::operator=;

        /** @brief Addition operator */
        template <class Vector>
        friend auto operator+(Vertex lhs, const Vector& rhs) -> Vertex
        {
            lhs += rhs;
            return lhs;
        }

        /** @brief Subtraction operator */
        template <class Vector>
        friend auto operator-(Vertex lhs, const Vector& rhs) -> Vertex
        {
            lhs -= rhs;
            return lhs;
        }

        /** @brief Multiplication operator */
        template <class Vector>
        friend auto operator*(Vertex lhs, const Vector& rhs) -> Vertex
        {
            lhs *= rhs;
            return lhs;
        }

        /** @brief Division operator */
        template <class Vector>
        friend auto operator/(Vertex lhs, const Vector& rhs) -> Vertex
        {
            lhs /= rhs;
            return lhs;
        }
    };

    /** @brief Face type */
    using Face = std::vector<std::size_t>;

    /** @brief Default constructor */
    Mesh() = default;

    /** Construct a new mesh */
    [[nodiscard]] static auto New() -> Pointer
    {
        return std::make_shared<Mesh>();
    }

    /**
     * @brief Insert a vertex
     *
     * Returns the index of the vertex in the mesh.
     */
    auto insertVertex(const Vertex& v) -> std::size_t
    {
        auto idx = vertices_.size();
        vertices_.push_back(v);
        adjacencyValid_ = false;
        faceNormalCache_.assign(faceNormalCache_.size(), std::nullopt);
        return idx;
    }

    /**
     * @brief Insert a vertex with element values
     *
     * The number of arguments provided must match Dims. Returns the index of
     * the vertex in the mesh.
     */
    template <typename... Args>
    auto insertVertex(Args... args) -> std::size_t
    {
        static_assert(sizeof...(args) == Dims, "Incorrect number of arguments");
        auto idx = vertices_.size();
        vertices_.emplace_back(args...);
        adjacencyValid_ = false;
        faceNormalCache_.assign(faceNormalCache_.size(), std::nullopt);
        return idx;
    }

    /** @brief Get a vertex by index */
    [[nodiscard]] auto vertex(std::size_t idx) const -> const Vertex&
    {
        return vertices_.at(idx);
    }

    /** @brief Get a vertex by index */
    [[nodiscard]] auto vertex(std::size_t idx) -> Vertex&
    {
        return vertices_.at(idx);
    }

    /**
     * @brief Insert a face
     *
     * Returns the index of the face in the mesh.
     */
    auto insertFace(const Face& f) -> std::size_t
    {
        auto idx = faces_.size();
        faces_.emplace_back(f);
        faceNormalCache_.emplace_back(std::nullopt);
        adjacencyValid_ = false;
        return idx;
    }

    /**
     * @brief Insert a face with vertex index values
     *
     * Returns the index of the face in the mesh.
     */
    template <typename... Indices>
    auto insertFace(Indices... indices) -> std::size_t
    {
        static_assert(sizeof...(indices) >= 3, "Face must have >= 3 vertices");
        auto idx = faces_.size();
        faces_.push_back(Face{static_cast<std::size_t>(indices)...});
        faceNormalCache_.emplace_back(std::nullopt);
        adjacencyValid_ = false;
        return idx;
    }

    /** @brief Get a face by index */
    [[nodiscard]] auto face(const std::size_t idx) const -> const Face&
    {
        return faces_.at(idx);
    }

    /** @brief Get a face by index */
    [[nodiscard]] auto face(const std::size_t idx) -> Face&
    {
        return faces_.at(idx);
    }

    /**
     * @brief Get the faces incident to a vertex
     *
     * Returns a reference to the list of face indices that contain the vertex
     * at @p idx. The adjacency index is built lazily on the first call and
     * invalidated whenever a vertex or face is inserted.
     *
     * @throws std::out_of_range if @p idx >= number of vertices
     */
    [[nodiscard]] auto vertexFaces(std::size_t idx) const
        -> const std::vector<std::size_t>&
    {
        if (idx >= vertices_.size()) {
            throw std::out_of_range("vertexFaces: vertex index out of range");
        }
        if (!adjacencyValid_) {
            buildAdjacency_();
        }
        return adjacency_[idx];
    }

    /**
     * @brief Get the unit normal of a face
     *
     * Computed lazily as @c normalize((v1-v0) x (v2-v0)) on first access and
     * cached in a parallel mutable vector. The cache is invalidated whenever
     * a vertex or face is inserted. Only defined for 3D meshes.
     *
     * @throws std::out_of_range if @p idx >= number of faces
     */
    [[nodiscard]] auto faceNormal(std::size_t idx) const -> Vec<T, Dims>
    {
        static_assert(Dims == 3, "faceNormal requires Dims == 3");
        if (idx >= faces_.size()) {
            throw std::out_of_range("faceNormal: face index out of range");
        }
        if (!faceNormalCache_[idx].has_value()) {
            const auto& f = faces_[idx];
            const auto& v0 = vertices_[f[0]];
            const auto& v1 = vertices_[f[1]];
            const auto& v2 = vertices_[f[2]];
            faceNormalCache_[idx] = normalize((v1 - v0).cross(v2 - v0));
        }
        return *faceNormalCache_[idx];
    }

private:
    /** Vertices */
    std::vector<Vertex> vertices_;
    /** Faces */
    std::vector<Face> faces_;

    /** Per-face normal cache (lazy, nullopt until first access, reset on mutation) */
    mutable std::vector<std::optional<Vec<T, Dims>>> faceNormalCache_;

    /** Vertex-to-face adjacency index (lazy, invalidated on mutation) */
    mutable std::vector<std::vector<std::size_t>> adjacency_;
    /** Whether adjacency_ is up to date */
    mutable bool adjacencyValid_{false};

    /** @brief (Re)build the vertex-to-face adjacency index */
    void buildAdjacency_() const
    {
        adjacency_.assign(vertices_.size(), std::vector<std::size_t>{});
        for (std::size_t fi = 0; fi < faces_.size(); ++fi) {
            for (auto vi : faces_[fi]) {
                adjacency_[vi].push_back(fi);
            }
        }
        adjacencyValid_ = true;
    }
};

/** @brief 3D 32-bit floating-point mesh */
using Mesh3f = Mesh<float, 3>;
/** @brief 3D 64-bit floating-point mesh */
using Mesh3d = Mesh<double, 3>;

}  // namespace educelab
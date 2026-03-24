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
#include "educelab/core/utils/Math.hpp"

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
    auto insert_vertex(const Vertex& v) -> std::size_t
    {
        const auto idx = vertices_.size();
        vertices_.push_back(v);
        // invalidate the adjacency and face normal cache
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
    auto insert_vertex(Args... args) -> std::size_t
    {
        static_assert(sizeof...(args) == Dims, "Incorrect number of arguments");
        const auto idx = vertices_.size();
        vertices_.emplace_back(args...);
        // invalidate the adjacency and face normal cache
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
    auto insert_face(const Face& f) -> std::size_t
    {
        const auto idx = faces_.size();
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
    auto insert_face(Indices... indices) -> std::size_t
    {
        static_assert(sizeof...(indices) >= 3, "Face must have >= 3 vertices");
        const auto idx = faces_.size();
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
    [[nodiscard]] auto vertex_faces(std::size_t idx) const
        -> const std::vector<std::size_t>&
    {
        if (idx >= vertices_.size()) {
            throw std::out_of_range("vertex_faces: vertex index out of range");
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
    [[nodiscard]] auto face_normal(std::size_t idx) const -> Vec<T, Dims>
    {
        static_assert(Dims == 3, "face_normal requires Dims == 3");
        if (idx >= faces_.size()) {
            throw std::out_of_range("face_normal: face index out of range");
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
            for (const auto vi : faces_[fi]) {
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

/**
 * @brief Compute an angle-weighted vertex normal
 *
 * Returns the normalized, angle-weighted average of the face normals incident
 * to vertex @p idx. The weight for each face is the interior angle of that
 * face at the given vertex. Uses the mesh's lazy adjacency index and face
 * normal cache; result is not cached. Only defined for 3D meshes.
 *
 * @tparam T   Numeric type of the mesh
 * @tparam Dims Must be 3
 * @tparam VertexTraits Vertex traits type
 */
template <
    typename T,
    std::size_t Dims,
    typename VertexTraits,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
[[nodiscard]] auto vertex_normal(
    const Mesh<T, Dims, VertexTraits>& mesh, std::size_t idx) -> Vec<T, Dims>
{
    static_assert(Dims == 3, "vertex_normal requires Dims == 3");

    Vec<T, Dims> weighted{};
    for (auto fi : mesh.vertex_faces(idx)) {
        const auto& f = mesh.face(fi);
        // Find position of idx within this face
        auto it = std::find(f.begin(), f.end(), idx);
        auto pos = static_cast<std::size_t>(std::distance(f.begin(), it));
        auto n = f.size();
        auto prev = f[(pos + n - 1) % n];
        auto next = f[(pos + 1) % n];
        const auto& v  = mesh.vertex(idx);
        const auto& vp = mesh.vertex(prev);
        const auto& vn = mesh.vertex(next);
        auto angle = interior_angle(vp - v, vn - v);
        weighted += mesh.face_normal(fi) * angle;
    }
    return normalize(weighted);
}

}  // namespace educelab
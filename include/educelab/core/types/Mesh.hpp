#pragma once

/** @file */

#include <algorithm>
#include <memory>
#include <optional>
#include <stdexcept>
#include <type_traits>
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
 * @note The @c std::optional storage has an @em unset state that I/O writers
 * must not fabricate; see the trait-author convention documented on
 * @ref has_any_normal.
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
 * @brief Detect whether a vertex type @c V carries a per-vertex normal
 *
 * Resolves to @c std::true_type when @c V has a @c .normal member (i.e. @c V
 * inherits @ref traits::WithNormal), @c std::false_type otherwise.
 *
 * I/O functions use this trait via @c if @c constexpr to conditionally read
 * or write normal data without requiring a separate function overload:
 *
 * @code
 * // Opt in by composing WithNormal into VertexTraits:
 * struct MyTraits : traits::WithNormal<float, 3> {};
 * using MyMesh = Mesh<float, 3, MyTraits>;
 *
 * // Detected automatically at compile time:
 * if constexpr (has_normal<typename MeshT::Vertex>::value) {
 *     // read/write vn lines
 * }
 * @endcode
 *
 * @tparam V Vertex type to inspect
 */
template <typename V, typename = void>
struct has_normal : std::false_type {
};

/** @cond */
template <typename V>
struct has_normal<V, std::void_t<decltype(std::declval<V>().normal)>>
    : std::true_type {
};
/** @endcond */

/**
 * @brief Opt-in vertex color mixin
 *
 * Compose into a custom traits struct via multiple inheritance to add
 * per-vertex color storage. Detected at compile time by I/O functions
 * via @c if @c constexpr and the C++17 detection idiom.
 *
 * @note @ref Color has an @em unset state that I/O writers must not fabricate;
 * see the trait-author convention documented on @ref has_any_normal.
 */
struct WithColor {
    /** @brief Vertex color */
    Color color;
};

/**
 * @brief Detect whether a vertex type @c V carries a per-vertex color
 *
 * Resolves to @c std::true_type when @c V has a @c .color member (i.e. @c V
 * inherits @ref traits::WithColor), @c std::false_type otherwise.
 *
 * I/O functions use this trait via @c if @c constexpr to conditionally read
 * or write inline vertex color data:
 *
 * @code
 * // Opt in by composing WithColor into VertexTraits:
 * struct MyTraits : traits::WithColor {};
 * using MyMesh = Mesh<float, 3, MyTraits>;
 *
 * // Detected automatically at compile time:
 * if constexpr (has_color<typename MeshT::Vertex>::value) {
 *     // OBJ: read/write inline 'v x y z r g b' lines
 *     // PLY: read/write 'red green blue' properties
 * }
 * @endcode
 *
 * @tparam V Vertex type to inspect
 */
template <typename V, typename = void>
struct has_color : std::false_type {
};

/** @cond */
template <typename V>
struct has_color<V, std::void_t<decltype(std::declval<V>().color)>>
    : std::true_type {
};
/** @endcond */

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

        /** Inherit value-assignment operator */
        using Vec<T, Dims>::operator=;

        /** @brief Addition-assignment operator */
        template <class Vector>
        auto operator+=(const Vector& rhs) -> Vertex&
        {
            Vec<T, Dims>::operator+=(rhs);
            return *this;
        }

        /** @brief Subtraction-assignment operator */
        template <class Vector>
        auto operator-=(const Vector& rhs) -> Vertex&
        {
            Vec<T, Dims>::operator-=(rhs);
            return *this;
        }

        /** @brief Scalar multiplication-assignment operator */
        template <class Scalar>
        auto operator*=(const Scalar& rhs) -> Vertex&
        {
            Vec<T, Dims>::operator*=(rhs);
            return *this;
        }

        /** @brief Scalar division-assignment operator */
        template <class Scalar>
        auto operator/=(const Scalar& rhs) -> Vertex&
        {
            Vec<T, Dims>::operator/=(rhs);
            return *this;
        }

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

        /** @brief Scalar multiplication operator */
        template <class Scalar>
        friend auto operator*(Vertex lhs, const Scalar& rhs) -> Vertex
        {
            lhs *= rhs;
            return lhs;
        }

        /** @brief Scalar division operator */
        template <class Scalar>
        friend auto operator/(Vertex lhs, const Scalar& rhs) -> Vertex
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
        adjacency_valid_ = false;
        face_normal_cache_.assign(face_normal_cache_.size(), std::nullopt);
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
        adjacency_valid_ = false;
        face_normal_cache_.assign(face_normal_cache_.size(), std::nullopt);
        return idx;
    }

    /** @brief Number of vertices in the mesh */
    [[nodiscard]] auto num_vertices() const noexcept -> std::size_t
    {
        return vertices_.size();
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
        face_normal_cache_.emplace_back(std::nullopt);
        adjacency_valid_ = false;
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
        face_normal_cache_.emplace_back(std::nullopt);
        adjacency_valid_ = false;
        return idx;
    }

    /** @brief Number of faces in the mesh */
    [[nodiscard]] auto num_faces() const noexcept -> std::size_t
    {
        return faces_.size();
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
        if (!adjacency_valid_) {
            build_adjacency();
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
        if (!face_normal_cache_[idx].has_value()) {
            const auto& f = faces_[idx];
            const auto& v0 = vertices_[f[0]];
            const auto& v1 = vertices_[f[1]];
            const auto& v2 = vertices_[f[2]];
            face_normal_cache_[idx] = normalize((v1 - v0).cross(v2 - v0));
        }
        return *face_normal_cache_[idx];
    }

    /** @brief Empty the mesh of all vertices and faces */
    auto clear() -> void
    {
        vertices_.clear();
        faces_.clear();
        face_normal_cache_.clear();
        adjacency_.clear();
        adjacency_valid_ = false;
    }

private:
    /** Vertices */
    std::vector<Vertex> vertices_;
    /** Faces */
    std::vector<Face> faces_;

    /** Per-face normal cache (lazy, nullopt until first access, reset on mutation) */
    mutable std::vector<std::optional<Vec<T, Dims>>> face_normal_cache_;

    /** Vertex-to-face adjacency index (lazy, invalidated on mutation) */
    mutable std::vector<std::vector<std::size_t>> adjacency_;
    /** Whether adjacency_ is up to date */
    mutable bool adjacency_valid_{false};

    /** @brief (Re)build the vertex-to-face adjacency index */
    void build_adjacency() const
    {
        adjacency_.assign(vertices_.size(), std::vector<std::size_t>{});
        for (std::size_t fi = 0; fi < faces_.size(); ++fi) {
            for (const auto vi : faces_[fi]) {
                adjacency_[vi].push_back(fi);
            }
        }
        adjacency_valid_ = true;
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
        auto nVerts = f.size();
        auto prev = f[(pos + nVerts - 1) % nVerts];
        auto next = f[(pos + 1) % nVerts];
        const auto& v     = mesh.vertex(idx);
        const auto& vPrev = mesh.vertex(prev);
        const auto& vNext = mesh.vertex(next);
        auto angle = interior_angle(vPrev - v, vNext - v);
        weighted += mesh.face_normal(fi) * angle;
    }
    return normalize(weighted);
}

/**
 * @brief Whether @p mesh actually carries any per-vertex normal
 *
 * Runtime companion to the compile-time @ref traits::has_normal trait:
 * @c has_normal reports whether the vertex type @em can hold a normal, while
 * @c has_any_normal reports whether at least one vertex @em does. Always
 * @c false when the vertex type has no normal member.
 *
 * I/O writers use this to avoid emitting fabricated zero normals for a
 * normal-capable mesh that has none set (OBJ would otherwise write
 * @c "vn 0 0 0"; PLY would declare @c nx/ny/nz and write zeros).
 *
 * @par Convention for trait authors
 * This is one instance of a general rule. A vertex/coordinate trait whose
 * storage carries a distinct @em unset state — an @c std::optional member, or
 * a type like @ref Color with its own empty state — must not be fabricated on
 * write. When you add such a trait, provide a @c has_any_&lt;trait&gt; runtime
 * helper alongside it (see @ref has_any_normal, @ref has_any_color) and gate
 * the I/O writers on it, so a capable-but-empty mesh round-trips as empty
 * rather than acquiring fabricated defaults (zeros, black, etc.). Traits whose
 * storage is a plain defaulted value where the default is itself valid (e.g.
 * @ref traits::WithChart's @c std::size_t @c chart, default @c 0 = first
 * chart) need no such guard — there is no unset state to misrepresent.
 *
 * @tparam T   Numeric type of the mesh
 * @tparam Dims Mesh dimensions
 * @tparam VertexTraits Vertex traits type
 */
template <
    typename T,
    std::size_t Dims,
    typename VertexTraits,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
[[nodiscard]] auto has_any_normal(const Mesh<T, Dims, VertexTraits>& mesh)
    -> bool
{
    using Vertex = typename Mesh<T, Dims, VertexTraits>::Vertex;
    if constexpr (traits::has_normal<Vertex>::value) {
        for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
            if (mesh.vertex(vi).normal.has_value()) {
                return true;
            }
        }
    }
    return false;
}

/**
 * @brief Whether @p mesh actually carries any per-vertex color
 *
 * Runtime companion to the compile-time @ref traits::has_color trait:
 * @c has_color reports whether the vertex type @em can hold a color, while
 * @c has_any_color reports whether at least one vertex @em does. Always
 * @c false when the vertex type has no color member.
 *
 * I/O writers use this to avoid emitting fabricated black for a color-capable
 * mesh that has none set (PLY would otherwise declare @c red/green/blue and
 * write @c "0 0 0"). OBJ gates inline RGB per vertex instead, since its
 * @c v lines carry color positionally and can vary per line.
 *
 * @tparam T   Numeric type of the mesh
 * @tparam Dims Mesh dimensions
 * @tparam VertexTraits Vertex traits type
 */
template <
    typename T,
    std::size_t Dims,
    typename VertexTraits,
    std::enable_if_t<std::is_arithmetic_v<T>, bool> = true>
[[nodiscard]] auto has_any_color(const Mesh<T, Dims, VertexTraits>& mesh)
    -> bool
{
    using Vertex = typename Mesh<T, Dims, VertexTraits>::Vertex;
    if constexpr (traits::has_color<Vertex>::value) {
        for (std::size_t vi = 0; vi < mesh.num_vertices(); ++vi) {
            if (mesh.vertex(vi).color.has_value()) {
                return true;
            }
        }
    }
    return false;
}

}  // namespace educelab
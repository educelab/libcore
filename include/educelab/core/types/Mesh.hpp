#pragma once

/** @file */

#include <memory>
#include <optional>
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
        faces_.emplace_back(indices...);
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

private:
    /** Vertices */
    std::vector<Vertex> vertices_;
    /** Faces */
    std::vector<Face> faces_;
};

/** @brief 3D 32-bit floating-point mesh */
using Mesh3f = Mesh<float, 3>;
/** @brief 3D 64-bit floating-point mesh */
using Mesh3d = Mesh<double, 3>;

}  // namespace educelab
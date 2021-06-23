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
 * @brief Default traits for Mesh vertices
 *
 * @tparam T Mesh numeric type
 * @tparam Dims Mesh dimensions
 */
template <
    typename T,
    std::size_t Dims,
    std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
struct DefaultVertexTraits {
    /** @brief Vertex normal */
    std::optional<Vec<T, Dims>> normal;

    /** @brief Vertex color */
    Color color;
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
    std::enable_if_t<std::is_arithmetic<T>::value, bool> = true>
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
        friend Vertex operator+(Vertex lhs, const Vector& rhs)
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
    [[nodiscard]] auto face(std::size_t idx) const -> const Face&
    {
        return faces_.at(idx);
    }

    /** @brief Get a face by index */
    [[nodiscard]] auto face(std::size_t idx) -> Face& { return faces_.at(idx); }

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
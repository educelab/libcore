#pragma once

/** @file */

#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

#include "educelab/core/types/Vec.hpp"

namespace educelab
{

namespace traits
{

/** @brief Default (empty) per-coordinate traits for UVMap */
struct DefaultUVTraits {
};

/**
 * @brief Opt-in chart-index mixin for UVMap coordinates
 *
 * Compose into a custom traits struct (or use directly) to attach a UV atlas
 * chart index to each coordinate:
 * @code
 * using MyUVMap = UVMap<float, 2, traits::WithChart>;
 * @endcode
 */
struct WithChart {
    /** @brief Atlas chart index */
    std::size_t chart{0};
};

/**
 * @brief Detect whether a UV map type @c UVMapT carries per-coordinate chart
 *        indices
 *
 * Resolves to @c std::true_type when @c UVMapT::Coordinate has a @c .chart
 * member (i.e. @c UVMapT is instantiated with @ref traits::WithChart),
 * @c std::false_type otherwise.
 *
 * I/O functions use this trait via @c if @c constexpr to conditionally
 * populate or preserve chart indices. @c write_obj with a @c
 * std::vector<std::filesystem::path> also enforces it at compile time via
 * @c static_assert:
 *
 * @code
 * // Opt in by instantiating UVMap with traits::WithChart:
 * using MyUVMap = UVMap<float, 2, traits::WithChart>;
 *
 * // Detected automatically at compile time:
 * if constexpr (traits::has_chart<UVMapT>::value) {
 *     // OBJ: populate chart indices from usemtl group ordering
 * }
 * @endcode
 *
 * @tparam UVMapT UVMap type to inspect
 */
template <typename UVMapT, typename = void>
struct has_chart : std::false_type {
};

/** @cond */
template <typename UVMapT>
struct has_chart<
    UVMapT,
    std::void_t<decltype(std::declval<typename UVMapT::Coordinate>().chart)>>
    : std::true_type {
};
/** @endcond */

}  // namespace traits

/**
 * @brief Per-wedge UV/UVW coordinate store
 *
 * Provides a two-level API mirroring the OBJ/PLY @c vt index structure: a
 * flat pool of @c Coordinate objects plus a per-face, per-corner index into
 * that pool. Multiple wedges may reference the same pool entry (no
 * duplication), and the same vertex can map to different UV coordinates in
 * adjacent faces (UV seam support).
 *
 * @tparam T      Floating-point element type (default: @c float)
 * @tparam Dims   Coordinate dimensionality (default: 2 for UV; use 3 for UVW)
 * @tparam Traits Optional per-coordinate metadata mixin (default:
 *                @ref traits::DefaultUVTraits)
 */
template <
    typename T = float,
    std::size_t Dims = 2,
    typename Traits = traits::DefaultUVTraits,
    std::enable_if_t<std::is_floating_point_v<T>, bool> = true>
class UVMap
{
public:
    /**
     * @brief Per-coordinate type inheriting @c Vec<T,Dims> and @c Traits
     *
     * Arithmetic operators are defined on @c Coordinate (not inherited from
     * @c Vec) so that both return @c Coordinate / @c Coordinate& rather than
     * @c Vec / @c Vec&, preserving any @c Traits fields across arithmetic.
     */
    struct Coordinate : public Vec<T, Dims>, public Traits {
        /** @brief Default constructor */
        Coordinate() = default;

        /**
         * @brief Construct with element values
         *
         * The number of arguments must match @c Dims.
         */
        template <typename... Args>
        explicit Coordinate(Args... args) : Vec<T, Dims>{args...}
        {
        }

        /** Inherit value-assignment operator from Vec */
        using Vec<T, Dims>::operator=;

        /** @brief Addition-assignment operator */
        template <class Vector>
        auto operator+=(const Vector& rhs) -> Coordinate&
        {
            Vec<T, Dims>::operator+=(rhs);
            return *this;
        }

        /** @brief Subtraction-assignment operator */
        template <class Vector>
        auto operator-=(const Vector& rhs) -> Coordinate&
        {
            Vec<T, Dims>::operator-=(rhs);
            return *this;
        }

        /** @brief Scalar multiplication-assignment operator */
        template <class Scalar>
        auto operator*=(const Scalar& rhs) -> Coordinate&
        {
            Vec<T, Dims>::operator*=(rhs);
            return *this;
        }

        /** @brief Scalar division-assignment operator */
        template <class Scalar>
        auto operator/=(const Scalar& rhs) -> Coordinate&
        {
            Vec<T, Dims>::operator/=(rhs);
            return *this;
        }

        /** @brief Addition operator */
        template <class Vector>
        friend auto operator+(Coordinate lhs, const Vector& rhs) -> Coordinate
        {
            lhs += rhs;
            return lhs;
        }

        /** @brief Subtraction operator */
        template <class Vector>
        friend auto operator-(Coordinate lhs, const Vector& rhs) -> Coordinate
        {
            lhs -= rhs;
            return lhs;
        }

        /** @brief Scalar multiplication operator */
        template <class Scalar>
        friend auto operator*(Coordinate lhs, const Scalar& rhs) -> Coordinate
        {
            lhs *= rhs;
            return lhs;
        }

        /** @brief Scalar division operator */
        template <class Scalar>
        friend auto operator/(Coordinate lhs, const Scalar& rhs) -> Coordinate
        {
            lhs /= rhs;
            return lhs;
        }
    };

    //--------------------------------------------------------------------------
    // UV coordinate pool
    //--------------------------------------------------------------------------

    /**
     * @brief Insert a coordinate into the pool
     *
     * @return Index of the inserted coordinate
     */
    [[nodiscard]] auto insert(const Coordinate& c) -> std::size_t
    {
        const auto idx = uvs_.size();
        uvs_.push_back(c);
        return idx;
    }

    /**
     * @brief Insert a coordinate into the pool (move)
     *
     * @return Index of the inserted coordinate
     */
    [[nodiscard]] auto insert(Coordinate&& c) -> std::size_t
    {
        const auto idx = uvs_.size();
        uvs_.push_back(std::move(c));
        return idx;
    }

    /**
     * @brief Insert a @c Vec<T,Dims> into the pool (constructs a default
     *        @c Coordinate with the Vec values)
     *
     * @return Index of the inserted coordinate
     */
    [[nodiscard]] auto insert(const Vec<T, Dims>& v) -> std::size_t
    {
        const auto idx = uvs_.size();
        Coordinate c;
        static_cast<Vec<T, Dims>&>(c) = v;
        uvs_.push_back(std::move(c));
        return idx;
    }

    /**
     * @brief Insert coordinate values directly (variadic)
     *
     * The number of arguments must equal @c Dims.
     *
     * @return Index of the inserted coordinate
     */
    template <typename... Args>
    [[nodiscard]] auto insert(Args... args) -> std::size_t
    {
        static_assert(
            sizeof...(Args) == Dims, "insert: argument count must equal Dims");
        const auto idx = uvs_.size();
        uvs_.emplace_back(args...);
        return idx;
    }

    /**
     * @brief Bounds-checked access to a pool coordinate (mutable)
     *
     * @throws std::out_of_range if @p idx >= pool size
     */
    [[nodiscard]] auto at(std::size_t idx) -> Coordinate&
    {
        return uvs_.at(idx);
    }

    /**
     * @brief Bounds-checked access to a pool coordinate (const)
     *
     * @throws std::out_of_range if @p idx >= pool size
     */
    [[nodiscard]] auto at(std::size_t idx) const -> const Coordinate&
    {
        return std::as_const(uvs_).at(idx);
    }

    //--------------------------------------------------------------------------
    // Per-wedge mapping
    //--------------------------------------------------------------------------

    /**
     * @brief Assign pool index @p uvIdx to wedge (face @p face, corner
     *        @p corner)
     *
     * Auto-grows storage if @p face or @p corner exceed current bounds.
     * Overwrites any existing mapping for the wedge.
     *
     * @pre @p face and @p corner must not equal @c std::numeric_limits<std::size_t>::max()
     * @pre @p uvIdx < size()
     */
    void map(std::size_t face, std::size_t corner, std::size_t uvIdx)
    {
        if (face >= face_uvs_.size()) {
            face_uvs_.resize(face + 1);
        }
        if (corner >= face_uvs_[face].size()) {
            face_uvs_[face].resize(corner + 1, std::nullopt);
        }
        face_uvs_[face][corner] = uvIdx;
    }

    /**
     * @brief Return the pool index for wedge (face, corner)
     *
     * @throws std::out_of_range if the wedge is out of range or unmapped
     */
    [[nodiscard]] auto get(std::size_t face, std::size_t corner) const
        -> std::size_t
    {
        if (face >= face_uvs_.size() || corner >= face_uvs_[face].size() ||
            !face_uvs_[face][corner].has_value()) {
            throw std::out_of_range("UVMap::get: wedge is unmapped");
        }
        return *face_uvs_[face][corner];
    }

    /**
     * @brief Convenience: return the coordinate for wedge (face, corner)
     *        (mutable)
     *
     * Equivalent to @c at(get(face, corner)).
     *
     * @throws std::out_of_range if the wedge is unmapped
     */
    [[nodiscard]] auto get_coordinate(std::size_t face, std::size_t corner)
        -> Coordinate&
    {
        return at(get(face, corner));
    }

    /**
     * @brief Convenience: return the coordinate for wedge (face, corner)
     *        (const)
     *
     * @throws std::out_of_range if the wedge is unmapped
     */
    [[nodiscard]] auto get_coordinate(
        std::size_t face, std::size_t corner) const -> const Coordinate&
    {
        return at(get(face, corner));
    }

    /**
     * @brief Return whether wedge (face, corner) has been mapped
     *
     * Returns @c false for any out-of-range index or unmapped slot.
     */
    [[nodiscard]] auto has(std::size_t face, std::size_t corner) const -> bool
    {
        if (face >= face_uvs_.size() || corner >= face_uvs_[face].size()) {
            return false;
        }
        return face_uvs_[face][corner].has_value();
    }

    //--------------------------------------------------------------------------
    // Container
    //--------------------------------------------------------------------------

    /** @brief Number of coordinates in the pool */
    [[nodiscard]] auto size() const noexcept -> std::size_t
    {
        return uvs_.size();
    }

    /** @brief Whether the coordinate pool is empty */
    [[nodiscard]] auto empty() const noexcept -> bool { return uvs_.empty(); }

    /** @brief Pre-allocate pool capacity */
    void reserve_uvs(std::size_t n) { uvs_.reserve(n); }

    /** @brief Pre-allocate outer face entries */
    void reserve_faces(std::size_t n) { face_uvs_.reserve(n); }

    /** @brief Reset pool and per-wedge mapping to empty */
    void clear() noexcept
    {
        uvs_.clear();
        face_uvs_.clear();
    }

private:
    /** UV coordinate pool */
    std::vector<Coordinate> uvs_;
    /**
     * Per-face, per-corner UV pool index; nullopt if unmapped.
     *
     * @todo The vector-of-vectors layout causes one heap allocation per face
     *       and pointer-chasing on every access. For large meshes a flat layout
     *       with an offset table would improve cache locality. A fixed-stride
     *       triangle-only variant is the right solution for real-time use cases.
     */
    std::vector<std::vector<std::optional<std::size_t>>> face_uvs_;
};

}  // namespace educelab

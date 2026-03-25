#pragma once

/** @file */

#include <type_traits>

namespace educelab
{

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
 * if constexpr (has_chart<UVMapT>::value) {
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

}  // namespace educelab

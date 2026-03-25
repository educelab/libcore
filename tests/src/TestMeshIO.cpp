#include <gtest/gtest.h>

#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"
#include "educelab/core/types/detail/MeshTraits.hpp"

using namespace educelab;

//------------------------------------------------------------------------------
// Compile-time detection: has_normal<V>
//
// Resolves to true_type when V has a `.normal` member (opt-in via
// traits::WithNormal), false_type otherwise.
//------------------------------------------------------------------------------

// Vertex types under test
using PlainMesh   = Mesh<float, 3>;                            // DefaultVertexTraits
using NormalMesh  = Mesh<float, 3, traits::WithNormal<float, 3>>;
using ColorMesh   = Mesh<float, 3, traits::WithColor>;

static_assert(
    !has_normal<PlainMesh::Vertex>::value,
    "DefaultVertexTraits vertex must not satisfy has_normal");

static_assert(
    has_normal<NormalMesh::Vertex>::value,
    "WithNormal vertex must satisfy has_normal");

static_assert(
    !has_normal<ColorMesh::Vertex>::value,
    "WithColor-only vertex must not satisfy has_normal");

//------------------------------------------------------------------------------
// Compile-time detection: has_color<V>
//
// Resolves to true_type when V has a `.color` member (opt-in via
// traits::WithColor), false_type otherwise.
//------------------------------------------------------------------------------

static_assert(
    !has_color<PlainMesh::Vertex>::value,
    "DefaultVertexTraits vertex must not satisfy has_color");

static_assert(
    !has_color<NormalMesh::Vertex>::value,
    "WithNormal-only vertex must not satisfy has_color");

static_assert(
    has_color<ColorMesh::Vertex>::value,
    "WithColor vertex must satisfy has_color");

//------------------------------------------------------------------------------
// Compile-time detection: has_chart<UVMapT>
//
// Resolves to true_type when UVMapT::Coordinate has a `.chart` member
// (opt-in via traits::WithChart), false_type otherwise.
//------------------------------------------------------------------------------

using PlainUVMap = UVMap<float, 2>;
using ChartUVMap = UVMap<float, 2, traits::WithChart>;

static_assert(
    !has_chart<PlainUVMap>::value,
    "Default UVMap must not satisfy has_chart");

static_assert(
    has_chart<ChartUVMap>::value,
    "UVMap<WithChart> must satisfy has_chart");

//------------------------------------------------------------------------------
// Combined traits: vertex with both normal and color
//------------------------------------------------------------------------------

struct NormalAndColor : traits::WithNormal<float, 3>, traits::WithColor {};

static_assert(
    has_normal<Mesh<float, 3, NormalAndColor>::Vertex>::value,
    "NormalAndColor vertex must satisfy has_normal");

static_assert(
    has_color<Mesh<float, 3, NormalAndColor>::Vertex>::value,
    "NormalAndColor vertex must satisfy has_color");

//------------------------------------------------------------------------------
// Runtime placeholder — the static_asserts above are the primary tests; this
// test exists so the binary links and the suite shows at least one test case.
//------------------------------------------------------------------------------

TEST(MeshTraits, StaticAssertionsPassAtCompileTime) {}

#include <gtest/gtest.h>

#include "educelab/core/types/UVMap.hpp"

using namespace educelab;

// Convenience aliases used throughout
using UVMap2f = UVMap<float, 2>;
using Coord2f = UVMap2f::Coordinate;

//------------------------------------------------------------------------------
// Coordinate construction and round-trip via insert/at
//------------------------------------------------------------------------------

TEST(UVMap, InsertCoordinateAndAtRoundTrip)
{
    UVMap2f m;
    Coord2f c;
    c[0] = 0.5f;
    c[1] = 0.25f;
    const auto idx = m.insert(c);
    EXPECT_EQ(m.at(idx)[0], 0.5f);
    EXPECT_EQ(m.at(idx)[1], 0.25f);
}

TEST(UVMap, InsertVariadicRoundTrip)
{
    UVMap2f m;
    const auto idx = m.insert(0.1f, 0.9f);
    EXPECT_EQ(m.at(idx)[0], 0.1f);
    EXPECT_EQ(m.at(idx)[1], 0.9f);
}

TEST(UVMap, InsertVecRoundTrip)
{
    UVMap2f m;
    Vec<float, 2> v;
    v[0] = 0.3f;
    v[1] = 0.7f;
    const auto idx = m.insert(v);
    EXPECT_EQ(m.at(idx)[0], 0.3f);
    EXPECT_EQ(m.at(idx)[1], 0.7f);
}

//------------------------------------------------------------------------------
// Mutable and const at()
//------------------------------------------------------------------------------

TEST(UVMap, AtNonConstReturnsMutableRef)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    Coord2f& ref = m.at(0);
    ref[0] = 0.8f;
    ref[1] = 0.2f;
    EXPECT_EQ(m.at(0)[0], 0.8f);
    EXPECT_EQ(m.at(0)[1], 0.2f);
}

TEST(UVMap, AtConstReturnsConstRef)
{
    UVMap2f m;
    m.insert(0.4f, 0.6f);
    const UVMap2f& cm = m;
    const Coord2f& ref = cm.at(0);
    // Implicit conversion to Vec verifies inheritance
    Vec<float, 2> v = ref;
    EXPECT_EQ(v[0], 0.4f);
    EXPECT_EQ(v[1], 0.6f);
}

TEST(UVMap, AtThrowsOutOfRange)
{
    UVMap2f m;
    EXPECT_THROW(m.at(0), std::out_of_range);
    m.insert(0.0f, 0.0f);
    EXPECT_THROW(m.at(1), std::out_of_range);
}

//------------------------------------------------------------------------------
// map / get on a triangle (3 corners)
//------------------------------------------------------------------------------

TEST(UVMap, MapAndGetTriangle)
{
    UVMap2f m;
    const auto uv0 = m.insert(0.0f, 0.0f);
    const auto uv1 = m.insert(1.0f, 0.0f);
    const auto uv2 = m.insert(0.0f, 1.0f);

    m.map(0, 0, uv0);
    m.map(0, 1, uv1);
    m.map(0, 2, uv2);

    EXPECT_EQ(m.get(0, 0), uv0);
    EXPECT_EQ(m.get(0, 1), uv1);
    EXPECT_EQ(m.get(0, 2), uv2);
}

//------------------------------------------------------------------------------
// map / get on a quad (4 corners)
//------------------------------------------------------------------------------

TEST(UVMap, MapAndGetQuad)
{
    UVMap2f m;
    const auto uv0 = m.insert(0.0f, 0.0f);
    const auto uv1 = m.insert(1.0f, 0.0f);
    const auto uv2 = m.insert(1.0f, 1.0f);
    const auto uv3 = m.insert(0.0f, 1.0f);

    m.map(0, 0, uv0);
    m.map(0, 1, uv1);
    m.map(0, 2, uv2);
    m.map(0, 3, uv3);

    EXPECT_EQ(m.get(0, 0), uv0);
    EXPECT_EQ(m.get(0, 1), uv1);
    EXPECT_EQ(m.get(0, 2), uv2);
    EXPECT_EQ(m.get(0, 3), uv3);
}

//------------------------------------------------------------------------------
// Corners mapped out of order
//------------------------------------------------------------------------------

TEST(UVMap, CornersOutOfOrder)
{
    UVMap2f m;
    const auto uv0 = m.insert(0.0f, 0.0f);
    const auto uv2 = m.insert(0.5f, 0.5f);

    m.map(0, 2, uv2);
    m.map(0, 0, uv0);

    EXPECT_EQ(m.get(0, 0), uv0);
    EXPECT_EQ(m.get(0, 2), uv2);
    // corner 1 was never mapped
    EXPECT_FALSE(m.has(0, 1));
}

//------------------------------------------------------------------------------
// has()
//------------------------------------------------------------------------------

TEST(UVMap, HasReturnsFalseForUnmappedCorner)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    m.map(0, 0, 0);
    EXPECT_FALSE(m.has(0, 1));
}

TEST(UVMap, HasReturnsFalseForOutOfRangeFace)
{
    UVMap2f m;
    EXPECT_FALSE(m.has(5, 0));
}

TEST(UVMap, HasReturnsFalseForOutOfRangeCorner)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    m.map(0, 0, 0);
    EXPECT_FALSE(m.has(0, 99));
}

TEST(UVMap, HasReturnsTrueForMappedWedge)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    m.map(0, 0, 0);
    EXPECT_TRUE(m.has(0, 0));
}

//------------------------------------------------------------------------------
// get() throws for unmapped wedge
//------------------------------------------------------------------------------

TEST(UVMap, GetThrowsForUnmappedCorner)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    m.map(0, 0, 0);
    EXPECT_THROW(m.get(0, 1), std::out_of_range);
}

TEST(UVMap, GetThrowsForOutOfRangeFace)
{
    UVMap2f m;
    EXPECT_THROW(m.get(99, 0), std::out_of_range);
}

//------------------------------------------------------------------------------
// get_coordinate
//------------------------------------------------------------------------------

TEST(UVMap, GetCoordinateNonConstReturnsMutableRef)
{
    UVMap2f m;
    m.insert(0.1f, 0.2f);
    m.map(0, 0, 0);

    Coord2f& ref = m.get_coordinate(0, 0);
    ref[0] = 0.9f;
    EXPECT_EQ(m.at(0)[0], 0.9f);
}

TEST(UVMap, GetCoordinateConstReturnsConstRef)
{
    UVMap2f m;
    m.insert(0.3f, 0.4f);
    m.map(0, 0, 0);

    const UVMap2f& cm = m;
    const Coord2f& ref = cm.get_coordinate(0, 0);
    EXPECT_EQ(ref[0], 0.3f);
    EXPECT_EQ(ref[1], 0.4f);
}

TEST(UVMap, GetCoordinateThrowsForUnmappedWedge)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    EXPECT_THROW(m.get_coordinate(0, 0), std::out_of_range);
}

//------------------------------------------------------------------------------
// Auto-grow: map beyond current storage
//------------------------------------------------------------------------------

TEST(UVMap, MapAutoGrowsFaceAndCorner)
{
    UVMap2f m;
    const auto idx = m.insert(0.5f, 0.5f);
    // face index 5, corner index 3 — well beyond initial empty storage
    m.map(5, 3, idx);
    EXPECT_TRUE(m.has(5, 3));
    EXPECT_EQ(m.get(5, 3), idx);
}

//------------------------------------------------------------------------------
// Two wedges sharing the same pool entry
//------------------------------------------------------------------------------

TEST(UVMap, TwoWedgesSharePoolEntry)
{
    UVMap2f m;
    const auto sharedIdx = m.insert(0.5f, 0.5f);
    m.map(0, 0, sharedIdx);
    m.map(1, 2, sharedIdx);

    EXPECT_EQ(m.get(0, 0), sharedIdx);
    EXPECT_EQ(m.get(1, 2), sharedIdx);
}

//------------------------------------------------------------------------------
// size() and empty()
//------------------------------------------------------------------------------

TEST(UVMap, SizeAndEmpty)
{
    UVMap2f m;
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0u);

    m.insert(0.0f, 0.0f);
    EXPECT_FALSE(m.empty());
    EXPECT_EQ(m.size(), 1u);

    m.insert(1.0f, 1.0f);
    EXPECT_EQ(m.size(), 2u);
}

//------------------------------------------------------------------------------
// reserve_uvs and reserve_faces do not change logical state
//------------------------------------------------------------------------------

TEST(UVMap, ReserveDoesNotChangeLogicalState)
{
    UVMap2f m;
    m.reserve_uvs(100);
    m.reserve_faces(50);
    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0u);
    EXPECT_FALSE(m.has(0, 0));
}

//------------------------------------------------------------------------------
// clear()
//------------------------------------------------------------------------------

TEST(UVMap, ClearResetsPoolAndMapping)
{
    UVMap2f m;
    m.insert(0.0f, 0.0f);
    m.map(0, 0, 0);

    m.clear();

    EXPECT_TRUE(m.empty());
    EXPECT_EQ(m.size(), 0u);
    EXPECT_FALSE(m.has(0, 0));
}

//------------------------------------------------------------------------------
// Copy semantics: copied map is independent
//------------------------------------------------------------------------------

TEST(UVMap, CopyIsIndependent)
{
    UVMap2f m;
    m.insert(0.1f, 0.2f);
    m.map(0, 0, 0);

    UVMap2f copy = m;
    copy.at(0)[0] = 0.9f;
    m.map(1, 0, 0);

    // Original pool entry unchanged
    EXPECT_EQ(m.at(0)[0], 0.1f);
    // Copy does not see original's new wedge mapping
    EXPECT_FALSE(copy.has(1, 0));
}

//------------------------------------------------------------------------------
// Coordinate arithmetic operators return Coordinate (not Vec)
//------------------------------------------------------------------------------

TEST(UVMap, CoordinateArithmeticPreservesType)
{
    // Verify that operator+ returns Coordinate, not Vec, so trait fields survive
    using Map = UVMap<float, 2, WithChart>;
    using Coord = Map::Coordinate;

    Coord a;
    a[0] = 0.1f;
    a[1] = 0.2f;
    a.chart = 3;

    Coord b;
    b[0] = 0.4f;
    b[1] = 0.6f;
    b.chart = 7;  // should be ignored by operator+ (only Vec data added)

    Coord sum = a + b;
    EXPECT_NEAR(sum[0], 0.5f, 1e-6f);
    EXPECT_NEAR(sum[1], 0.8f, 1e-6f);

    Coord scaled = a * 2.0f;
    EXPECT_NEAR(scaled[0], 0.2f, 1e-6f);
    EXPECT_NEAR(scaled[1], 0.4f, 1e-6f);
}

TEST(UVMap, CompoundAssignmentReturnsCoordsRef)
{
    using Map = UVMap<float, 2, WithChart>;
    using Coord = Map::Coordinate;

    Coord a;
    a[0] = 0.5f;
    a[1] = 0.5f;
    a.chart = 42;

    Coord b;
    b[0] = 0.1f;
    b[1] = 0.1f;

    Coord& ref = (a += b);
    // ref must alias a
    ref.chart = 99;
    EXPECT_EQ(a.chart, 99u);
    EXPECT_NEAR(a[0], 0.6f, 1e-6f);
}

//------------------------------------------------------------------------------
// WithChart trait: insert preserves chart field
//------------------------------------------------------------------------------

TEST(UVMap, WithChartTraitPreservedByAt)
{
    UVMap<float, 2, WithChart> m;
    UVMap<float, 2, WithChart>::Coordinate c;
    c[0] = 0.5f;
    c[1] = 0.5f;
    c.chart = 7;
    const auto idx = m.insert(c);
    EXPECT_EQ(m.at(idx).chart, 7u);
}

//------------------------------------------------------------------------------
// UVMap<double, 3> for UVW coordinates
//------------------------------------------------------------------------------

TEST(UVMap, UVWDoubleRoundTrip)
{
    UVMap<double, 3> m;
    const auto idx = m.insert(0.1, 0.2, 0.3);
    EXPECT_DOUBLE_EQ(m.at(idx)[0], 0.1);
    EXPECT_DOUBLE_EQ(m.at(idx)[1], 0.2);
    EXPECT_DOUBLE_EQ(m.at(idx)[2], 0.3);
}

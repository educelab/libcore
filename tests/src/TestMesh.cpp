#include <gtest/gtest.h>

#include "educelab/core/types/Color.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

using Vertex = Mesh3f::Vertex;

TEST(Vertex, OperatorPlus)
{
    Vertex a{1, 1, 1};
    Vertex b{1, 1, 1};
    EXPECT_EQ(a + b, Vertex(2, 2, 2));
    EXPECT_EQ(a, Vertex(1, 1, 1));
    EXPECT_EQ(b, Vertex(1, 1, 1));
    EXPECT_EQ(a += b, Vertex(2, 2, 2));
    EXPECT_EQ(a, Vertex(2, 2, 2));
}

TEST(Vertex, OperatorMinus)
{
    Vertex a{1, 1, 1};
    Vertex b{1, 1, 1};
    EXPECT_EQ(a - b, Vertex(0, 0, 0));
    EXPECT_EQ(a, Vertex(1, 1, 1));
    EXPECT_EQ(b, Vertex(1, 1, 1));
    EXPECT_EQ(a -= b, Vertex(0, 0, 0));
    EXPECT_EQ(a, Vertex(0, 0, 0));
}

TEST(Vertex, OperatorMultiply)
{
    Vertex a{1, 1, 1};
    EXPECT_EQ(a * 2, Vertex(2, 2, 2));
    EXPECT_EQ(a, Vertex(1, 1, 1));
    EXPECT_EQ(a *= 2, Vertex(2, 2, 2));
    EXPECT_EQ(a, Vertex(2, 2, 2));
}

TEST(Vertex, OperatorDivide)
{
    Vertex a{2, 2, 2};
    EXPECT_EQ(a / 2, Vertex(1, 1, 1));
    EXPECT_EQ(a, Vertex(2, 2, 2));
    EXPECT_EQ(a /= 2, Vertex(1, 1, 1));
    EXPECT_EQ(a, Vertex(1, 1, 1));
}

TEST(Vertex, NormalTrait)
{
    // Check normal
    Vertex v;
    Vec3f expected_normal{0, 1, 0};
    EXPECT_FALSE(v.normal.has_value());
    v.normal = expected_normal;
    EXPECT_TRUE(v.normal.has_value());
    EXPECT_EQ(v.normal, expected_normal);
}

TEST(Vertex, ColorTrait)
{
    // Check normal
    Vertex v;
    Color expected_color{uint8_t(255)};
    EXPECT_FALSE(v.color.has_value());
    v.color = expected_color;
    EXPECT_TRUE(v.color.has_value());
    EXPECT_EQ(v.color, expected_color);
}

TEST(Mesh, AddVerticesAndFace)
{
    // Build mesh
    Mesh3f mesh;
    std::vector<float> expectedVerts{10, 12, 13};
    Mesh3f::Face expectedFace;
    for (const auto& i : expectedVerts) {
        expectedFace.emplace_back(mesh.insertVertex(i, i, i));
    }
    auto faceIdx = mesh.insertFace(expectedFace);

    // Check vertices
    Vec3f expected;
    for (const auto& [idx, vertIdx] : enumerate(expectedFace)) {
        expected.fill(expectedVerts[idx]);
        EXPECT_EQ(mesh.vertex(vertIdx), expected);
    }

    // Check face
    EXPECT_EQ(mesh.face(faceIdx), expectedFace);
}

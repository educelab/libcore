#include <gtest/gtest.h>

#include "educelab/core/types/Color.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

// Custom traits composing both opt-in mixins
struct MyTraits : traits::WithNormal<float, 3>, traits::WithColor {};
using TestMesh = Mesh<float, 3, MyTraits>;
using TestVertex = TestMesh::Vertex;

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
    TestVertex v;
    Vec3f expected_normal{0, 1, 0};
    EXPECT_FALSE(v.normal.has_value());
    v.normal = expected_normal;
    EXPECT_TRUE(v.normal.has_value());
    EXPECT_EQ(v.normal, expected_normal);
}

TEST(Vertex, ColorTrait)
{
    TestVertex v;
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
    const std::vector<float> expectedVerts{10, 12, 13};
    Mesh3f::Face expectedFace;
    for (const auto& i : expectedVerts) {
        expectedFace.emplace_back(mesh.insertVertex(i, i, i));
    }
    const auto faceIdx = mesh.insertFace(expectedFace);

    // Check vertices
    Vec3f expected;
    for (const auto& [idx, vertIdx] : enumerate(expectedFace)) {
        expected.fill(expectedVerts[idx]);
        EXPECT_EQ(mesh.vertex(vertIdx), expected);
    }

    // Check face
    EXPECT_EQ(mesh.face(faceIdx), expectedFace);
}

TEST(Mesh, VertexFacesSingleFace)
{
    Mesh3f mesh;
    auto v0 = mesh.insertVertex(0, 0, 0);
    auto v1 = mesh.insertVertex(1, 0, 0);
    auto v2 = mesh.insertVertex(0, 1, 0);
    auto f0 = mesh.insertFace(v0, v1, v2);

    EXPECT_EQ(mesh.vertexFaces(v0), (std::vector<std::size_t>{f0}));
    EXPECT_EQ(mesh.vertexFaces(v1), (std::vector<std::size_t>{f0}));
    EXPECT_EQ(mesh.vertexFaces(v2), (std::vector<std::size_t>{f0}));
}

TEST(Mesh, VertexFacesSharedVertex)
{
    // Two triangles sharing an edge (v1-v2)
    Mesh3f mesh;
    auto v0 = mesh.insertVertex(0, 0, 0);
    auto v1 = mesh.insertVertex(1, 0, 0);
    auto v2 = mesh.insertVertex(0, 1, 0);
    auto v3 = mesh.insertVertex(1, 1, 0);
    auto f0 = mesh.insertFace(v0, v1, v2);
    auto f1 = mesh.insertFace(v1, v3, v2);

    // Shared vertices appear in both faces
    const auto& v1Faces = mesh.vertexFaces(v1);
    ASSERT_EQ(v1Faces.size(), 2u);
    EXPECT_TRUE(
        std::find(v1Faces.begin(), v1Faces.end(), f0) != v1Faces.end());
    EXPECT_TRUE(
        std::find(v1Faces.begin(), v1Faces.end(), f1) != v1Faces.end());

    // Non-shared vertex appears in only one face
    EXPECT_EQ(mesh.vertexFaces(v0), (std::vector<std::size_t>{f0}));
    EXPECT_EQ(mesh.vertexFaces(v3), (std::vector<std::size_t>{f1}));
}

TEST(Mesh, VertexFacesRebuiltAfterInsert)
{
    Mesh3f mesh;
    auto v0 = mesh.insertVertex(0, 0, 0);
    auto v1 = mesh.insertVertex(1, 0, 0);
    auto v2 = mesh.insertVertex(0, 1, 0);
    mesh.insertFace(v0, v1, v2);

    // Prime the cache
    EXPECT_EQ(mesh.vertexFaces(v0).size(), 1u);

    // Add a new face referencing v0; cache must be rebuilt
    auto v3 = mesh.insertVertex(1, 1, 0);
    auto f1 = mesh.insertFace(v0, v2, v3);

    EXPECT_EQ(mesh.vertexFaces(v0).size(), 2u);
    const auto& v0Faces = mesh.vertexFaces(v0);
    EXPECT_TRUE(
        std::find(v0Faces.begin(), v0Faces.end(), f1) != v0Faces.end());
}

TEST(Mesh, VertexFacesOutOfRange)
{
    Mesh3f mesh;
    EXPECT_THROW(mesh.vertexFaces(0), std::out_of_range);

    mesh.insertVertex(0, 0, 0);
    EXPECT_THROW(mesh.vertexFaces(1), std::out_of_range);
}

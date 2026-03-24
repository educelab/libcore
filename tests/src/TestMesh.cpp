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
    const Vertex b{1, 1, 1};
    EXPECT_EQ(a + b, Vertex(2, 2, 2));
    EXPECT_EQ(a, Vertex(1, 1, 1));
    EXPECT_EQ(b, Vertex(1, 1, 1));
    EXPECT_EQ(a += b, Vertex(2, 2, 2));
    EXPECT_EQ(a, Vertex(2, 2, 2));
}

TEST(Vertex, OperatorMinus)
{
    Vertex a{1, 1, 1};
    const Vertex b{1, 1, 1};
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
        expectedFace.emplace_back(mesh.insert_vertex(i, i, i));
    }
    const auto faceIdx = mesh.insert_face(expectedFace);

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
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    const auto f0 = mesh.insert_face(v0, v1, v2);

    EXPECT_EQ(mesh.vertex_faces(v0), (std::vector<std::size_t>{f0}));
    EXPECT_EQ(mesh.vertex_faces(v1), (std::vector<std::size_t>{f0}));
    EXPECT_EQ(mesh.vertex_faces(v2), (std::vector<std::size_t>{f0}));
}

TEST(Mesh, VertexFacesSharedVertex)
{
    // Two triangles sharing an edge (v1-v2)
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    const auto v3 = mesh.insert_vertex(1, 1, 0);
    const auto f0 = mesh.insert_face(v0, v1, v2);
    const auto f1 = mesh.insert_face(v1, v3, v2);

    // Shared vertices appear in both faces
    const auto& v1Faces = mesh.vertex_faces(v1);
    ASSERT_EQ(v1Faces.size(), 2u);
    EXPECT_TRUE(
        std::find(v1Faces.begin(), v1Faces.end(), f0) != v1Faces.end());
    EXPECT_TRUE(
        std::find(v1Faces.begin(), v1Faces.end(), f1) != v1Faces.end());

    // Non-shared vertex appears in only one face
    EXPECT_EQ(mesh.vertex_faces(v0), (std::vector<std::size_t>{f0}));
    EXPECT_EQ(mesh.vertex_faces(v3), (std::vector<std::size_t>{f1}));
}

TEST(Mesh, VertexFacesRebuiltAfterInsert)
{
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    mesh.insert_face(v0, v1, v2);

    // Prime the cache
    EXPECT_EQ(mesh.vertex_faces(v0).size(), 1u);

    // Add a new face referencing v0; cache must be rebuilt
    const auto v3 = mesh.insert_vertex(1, 1, 0);
    const auto f1 = mesh.insert_face(v0, v2, v3);

    EXPECT_EQ(mesh.vertex_faces(v0).size(), 2u);
    const auto& v0Faces = mesh.vertex_faces(v0);
    EXPECT_TRUE(
        std::find(v0Faces.begin(), v0Faces.end(), f1) != v0Faces.end());
}

TEST(Mesh, VertexFacesOutOfRange)
{
    Mesh3f mesh;
    EXPECT_THROW(mesh.vertex_faces(0), std::out_of_range);

    mesh.insert_vertex(0, 0, 0);
    EXPECT_THROW(mesh.vertex_faces(1), std::out_of_range);
}

TEST(Mesh, FaceNormalKnownTriangle)
{
    // Triangle in XY plane: normal should point in +Z
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    const auto f0 = mesh.insert_face(v0, v1, v2);

    auto n = mesh.face_normal(f0);
    EXPECT_NEAR(n[0], 0.f, 1e-6f);
    EXPECT_NEAR(n[1], 0.f, 1e-6f);
    EXPECT_NEAR(n[2], 1.f, 1e-6f);
}

TEST(Mesh, FaceNormalCacheHit)
{
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    const auto f0 = mesh.insert_face(v0, v1, v2);

    // Two calls must return the same value (cache hit on second call)
    const auto n0 = mesh.face_normal(f0);
    const auto n1 = mesh.face_normal(f0);
    EXPECT_EQ(n0, n1);
}

TEST(Mesh, FaceNormalInvalidatedAfterInsertFace)
{
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    const auto f0 = mesh.insert_face(v0, v1, v2);

    // Prime the cache
    const auto n0 = mesh.face_normal(f0);

    // Add a new face — cache should be invalidated but f0 still correct
    const auto v3 = mesh.insert_vertex(0, 0, 1);
    mesh.insert_face(v0, v1, v3);

    const auto n1 = mesh.face_normal(f0);
    EXPECT_EQ(n0, n1);
}

TEST(Mesh, FaceNormalInvalidatedAfterInsertVertex)
{
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    const auto f0 = mesh.insert_face(v0, v1, v2);

    // Prime the cache
    const auto n0 = mesh.face_normal(f0);

    // Insert a vertex — cache must be invalidated; f0 normal unchanged
    mesh.insert_vertex(5, 5, 5);
    const auto n1 = mesh.face_normal(f0);
    EXPECT_EQ(n0, n1);
}

TEST(Mesh, VertexNormalSingleFace)
{
    // A single XY-plane triangle: all vertices should have normal +Z
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 1, 0);
    mesh.insert_face(v0, v1, v2);

    auto n = vertex_normal(mesh, v0);
    EXPECT_NEAR(n[0], 0.f, 1e-5f);
    EXPECT_NEAR(n[1], 0.f, 1e-5f);
    EXPECT_NEAR(n[2], 1.f, 1e-5f);
}

TEST(Mesh, VertexNormalSharedVertexWeighted)
{
    // Two right triangles sharing an edge, both in XY plane — shared vertex
    // receives contributions from both faces; result should still be +Z
    Mesh3f mesh;
    auto v0 = mesh.insert_vertex(0, 0, 0);
    auto v1 = mesh.insert_vertex(1, 0, 0);
    auto v2 = mesh.insert_vertex(0, 1, 0);
    auto v3 = mesh.insert_vertex(1, 1, 0);
    mesh.insert_face(v0, v1, v2);
    mesh.insert_face(v1, v3, v2);

    // All vertices should have normal pointing in +Z
    for (auto vi : {v0, v1, v2, v3}) {
        auto n = vertex_normal(mesh, vi);
        EXPECT_NEAR(n[2], 1.f, 1e-5f) << "vertex " << vi;
    }
}

TEST(Mesh, VertexNormalBoundaryVertex)
{
    // A vertex that only belongs to one face — should equal the face normal
    Mesh3f mesh;
    const auto v0 = mesh.insert_vertex(0, 0, 0);
    const auto v1 = mesh.insert_vertex(1, 0, 0);
    const auto v2 = mesh.insert_vertex(0, 0, 1);
    mesh.insert_face(v0, v1, v2);

    auto fn = mesh.face_normal(0);
    auto vn = vertex_normal(mesh, v0);
    for (std::size_t i = 0; i < 3; ++i) {
        EXPECT_NEAR(vn[i], fn[i], 1e-5f);
    }
}

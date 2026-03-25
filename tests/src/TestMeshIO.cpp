#include <gtest/gtest.h>

#include <filesystem>

#include "educelab/core/io/MeshIO_OBJ.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"
#include "educelab/core/types/detail/MeshTraits.hpp"

namespace fs = std::filesystem;
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

//------------------------------------------------------------------------------
// OBJ Round-Trip Test Fixture
//------------------------------------------------------------------------------

class OBJTest : public ::testing::Test
{
protected:
    fs::path dir;

    void SetUp() override
    {
        dir = fs::temp_directory_path() / "educelab_core_meshio_objtest";
        fs::create_directories(dir);
    }

    void TearDown() override { fs::remove_all(dir); }

    fs::path obj(const std::string& name) const
    {
        return dir / (name + ".obj");
    }
};

// Mesh type aliases used throughout
using NormalMesh  = Mesh<float, 3, traits::WithNormal<float, 3>>;
using ColorMesh   = Mesh<float, 3, traits::WithColor>;
struct NCTraits : traits::WithNormal<float, 3>, traits::WithColor {};
using NCMesh     = Mesh<float, 3, NCTraits>;
using UVMap2f    = UVMap<float, 2>;
using ChartUVMap = UVMap<float, 2, traits::WithChart>;

//------------------------------------------------------------------------------
// Helpers to build small canonical meshes
//------------------------------------------------------------------------------

// Triangle: v0=(0,0,0) v1=(1,0,0) v2=(0,1,0)
static auto make_triangle() -> Mesh3f
{
    Mesh3f m;
    (void)m.insert_vertex(0.f, 0.f, 0.f);
    (void)m.insert_vertex(1.f, 0.f, 0.f);
    (void)m.insert_vertex(0.f, 1.f, 0.f);
    (void)m.insert_face(0u, 1u, 2u);
    return m;
}

// Quad: v0..v3 as unit square in XY-plane
static auto make_quad() -> Mesh3f
{
    Mesh3f m;
    (void)m.insert_vertex(0.f, 0.f, 0.f);
    (void)m.insert_vertex(1.f, 0.f, 0.f);
    (void)m.insert_vertex(1.f, 1.f, 0.f);
    (void)m.insert_vertex(0.f, 1.f, 0.f);
    (void)m.insert_face(0u, 1u, 2u, 3u);
    return m;
}

// Simple per-wedge UVMap for a single-face triangle
static auto make_triangle_uvmap(const Mesh3f& /*m*/) -> UVMap2f
{
    UVMap2f uv;
    (void)uv.insert(0.f, 0.f);  // pool index 0 → corner 0
    (void)uv.insert(1.f, 0.f);  // pool index 1 → corner 1
    (void)uv.insert(0.f, 1.f);  // pool index 2 → corner 2
    uv.map(0, 0, 0);
    uv.map(0, 1, 1);
    uv.map(0, 2, 2);
    return uv;
}

//------------------------------------------------------------------------------
// Task 2.1 — OBJ Round-Trip Tests
//------------------------------------------------------------------------------

TEST_F(OBJTest, PositionsOnly)
{
    const auto src = make_triangle();
    const auto path = obj("positions");
    write_obj(path, src);

    Mesh3f dst;
    read_obj(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_EQ(dst.num_faces(), 1u);
    EXPECT_NEAR(dst.vertex(0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[1], 1.f, 1e-5f);
    EXPECT_EQ(dst.face(0), (Mesh3f::Face{0, 1, 2}));
}

TEST_F(OBJTest, PositionsWithNormals)
{
    NormalMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).normal = Vec3f{0, 0, 1};
    src.vertex(1).normal = Vec3f{0, 0, 1};
    src.vertex(2).normal = Vec3f{0, 0, 1};
    src.insert_face(0u, 1u, 2u);

    const auto path = obj("normals");
    write_obj(path, src);

    NormalMesh dst;
    read_obj(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).normal.has_value());
    EXPECT_NEAR((*dst.vertex(0).normal)[2], 1.f, 1e-5f);
    EXPECT_NEAR((*dst.vertex(2).normal)[2], 1.f, 1e-5f);
}

TEST_F(OBJTest, PositionsWithColors_InlineRGB)
{
    ColorMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).color = Color::F32C3{1.f, 0.f, 0.f};
    src.vertex(1).color = Color::F32C3{0.f, 1.f, 0.f};
    src.vertex(2).color = Color::F32C3{0.f, 0.f, 1.f};
    src.insert_face(0u, 1u, 2u);

    const auto path = obj("colors");
    write_obj(path, src);

    // Verify file contains inline RGB on v lines
    std::ifstream f(path);
    std::string line;
    bool found_rgb = false;
    while (std::getline(f, line)) {
        if (line.rfind("v ", 0) == 0) {
            const auto toks = split(std::string_view(line));
            if (toks.size() >= 7) {
                found_rgb = true;
            }
        }
    }
    EXPECT_TRUE(found_rgb) << "Expected 'v x y z r g b' format";

    // Round-trip: read back and verify colors
    ColorMesh dst;
    read_obj(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).color.has_value());
    const auto c0 = dst.vertex(0).color.value<Color::F32C3>();
    EXPECT_NEAR(c0[0], 1.f, 1e-5f);
    EXPECT_NEAR(c0[1], 0.f, 1e-5f);
    EXPECT_NEAR(c0[2], 0.f, 1e-5f);
}

TEST_F(OBJTest, PositionsWithUVs_NoTexture)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto path     = obj("uvs");
    write_obj(path, src_mesh, src_uv);

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    read_obj(path, dst_mesh, dst_uv);

    ASSERT_EQ(dst_mesh.num_vertices(), 3u);
    ASSERT_EQ(dst_uv.size(), 3u);
    EXPECT_NEAR(dst_uv.at(0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(2)[1], 1.f, 1e-5f);
    // Wedge mapping round-trips
    EXPECT_EQ(dst_uv.get(0, 0), 0u);
    EXPECT_EQ(dst_uv.get(0, 1), 1u);
    EXPECT_EQ(dst_uv.get(0, 2), 2u);
}

TEST_F(OBJTest, PositionsWithUVs_SingleTexture_MTLWritten)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto tex_path = fs::path("texture.png");
    const auto path     = obj("single_tex");
    write_obj(path, src_mesh, src_uv, tex_path);

    // MTL file must exist alongside the OBJ
    const auto mtl_path = fs::path(path).replace_extension(".mtl");
    ASSERT_TRUE(fs::exists(mtl_path)) << "MTL file not written";

    // Round-trip: read back texture_paths
    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_obj(path, dst_mesh, dst_uv, dst_textures);

    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_EQ(dst_textures[0], tex_path);
}

TEST_F(OBJTest, PositionsWithUVs_MultiChart_PerChartUsemtl)
{
    // Two triangles; each on its own chart
    Mesh3f src_mesh;
    (void)src_mesh.insert_vertex(0.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(1.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(0.f, 1.f, 0.f);
    (void)src_mesh.insert_vertex(2.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(3.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(2.f, 1.f, 0.f);
    (void)src_mesh.insert_face(0u, 1u, 2u);  // face 0 → chart 0
    (void)src_mesh.insert_face(3u, 4u, 5u);  // face 1 → chart 1

    ChartUVMap src_uv;
    // Chart 0 UVs (pool 0..2)
    (void)src_uv.insert(ChartUVMap::Coordinate{0.f, 0.f});
    src_uv.at(0).chart = 0;
    (void)src_uv.insert(ChartUVMap::Coordinate{1.f, 0.f});
    src_uv.at(1).chart = 0;
    (void)src_uv.insert(ChartUVMap::Coordinate{0.f, 1.f});
    src_uv.at(2).chart = 0;
    // Chart 1 UVs (pool 3..5)
    (void)src_uv.insert(ChartUVMap::Coordinate{0.5f, 0.f});
    src_uv.at(3).chart = 1;
    (void)src_uv.insert(ChartUVMap::Coordinate{1.f, 0.5f});
    src_uv.at(4).chart = 1;
    (void)src_uv.insert(ChartUVMap::Coordinate{0.f, 0.5f});
    src_uv.at(5).chart = 1;
    src_uv.map(0, 0, 0);  src_uv.map(0, 1, 1);  src_uv.map(0, 2, 2);
    src_uv.map(1, 0, 3);  src_uv.map(1, 1, 4);  src_uv.map(1, 2, 5);

    const std::vector<fs::path> tex_paths{
        fs::path("chart0.png"), fs::path("chart1.png")};
    const auto path = obj("multicharts");
    write_obj(path, src_mesh, src_uv, tex_paths);

    // Verify file contains two usemtl directives
    std::ifstream f(path);
    std::string line;
    int usemtl_count = 0;
    while (std::getline(f, line)) {
        if (line.rfind("usemtl", 0) == 0) {
            ++usemtl_count;
        }
    }
    EXPECT_EQ(usemtl_count, 2);

    // Round-trip into a WithChart UVMap
    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> dst_textures;
    read_obj(path, dst_mesh, dst_uv, dst_textures);

    ASSERT_EQ(dst_textures.size(), 2u);
    EXPECT_EQ(dst_textures[0], tex_paths[0]);
    EXPECT_EQ(dst_textures[1], tex_paths[1]);
    // Chart indices recovered
    EXPECT_EQ(dst_uv.at(dst_uv.get(0, 0)).chart, 0u);
    EXPECT_EQ(dst_uv.at(dst_uv.get(1, 0)).chart, 1u);
}

TEST_F(OBJTest, ReadMultiChart_ChartIndicesOnlyWhenHasChart)
{
    // Write a multi-chart OBJ (reuse the two-triangle setup)
    Mesh3f src_mesh;
    (void)src_mesh.insert_vertex(0.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(1.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(0.f, 1.f, 0.f);
    (void)src_mesh.insert_vertex(2.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(3.f, 0.f, 0.f);
    (void)src_mesh.insert_vertex(2.f, 1.f, 0.f);
    (void)src_mesh.insert_face(0u, 1u, 2u);
    (void)src_mesh.insert_face(3u, 4u, 5u);

    ChartUVMap src_uv;
    for (int i = 0; i < 3; ++i) {
        (void)src_uv.insert(ChartUVMap::Coordinate{});
        src_uv.at(static_cast<std::size_t>(i)).chart = 0;
    }
    for (int i = 3; i < 6; ++i) {
        (void)src_uv.insert(ChartUVMap::Coordinate{});
        src_uv.at(static_cast<std::size_t>(i)).chart = 1;
    }
    src_uv.map(0, 0, 0); src_uv.map(0, 1, 1); src_uv.map(0, 2, 2);
    src_uv.map(1, 0, 3); src_uv.map(1, 1, 4); src_uv.map(1, 2, 5);

    const std::vector<fs::path> tex_paths{
        fs::path("a.png"), fs::path("b.png")};
    const auto path = obj("chart_no_trait");
    write_obj(path, src_mesh, src_uv, tex_paths);

    // Read into a plain UVMap (no WithChart) — must not crash
    Mesh3f dst_mesh;
    UVMap2f dst_uv;  // no WithChart trait
    EXPECT_NO_THROW(read_obj(path, dst_mesh, dst_uv));
    // UVs are still populated even without chart trait
    EXPECT_EQ(dst_uv.size(), 6u);
}

TEST_F(OBJTest, NGonFace_Quad)
{
    const auto src = make_quad();
    const auto path = obj("quad");
    write_obj(path, src);

    Mesh3f dst;
    read_obj(path, dst);

    ASSERT_EQ(dst.num_vertices(), 4u);
    ASSERT_EQ(dst.num_faces(), 1u);
    EXPECT_EQ(dst.face(0).size(), 4u);
}

TEST_F(OBJTest, ReadNormals_SilentlyIgnored_WhenMeshHasNoNormals)
{
    // Write a file that has vn lines
    NormalMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).normal = Vec3f{0, 0, 1};
    src.vertex(1).normal = Vec3f{0, 0, 1};
    src.vertex(2).normal = Vec3f{0, 0, 1};
    src.insert_face(0u, 1u, 2u);

    const auto path = obj("normals_ignored");
    write_obj(path, src);

    // Read into plain Mesh3f (no WithNormal) — must not crash, normals ignored
    Mesh3f dst;
    EXPECT_NO_THROW(read_obj(path, dst));
    ASSERT_EQ(dst.num_vertices(), 3u);
}

TEST_F(OBJTest, ReadMissingFile_Throws)
{
    Mesh3f dst;
    EXPECT_THROW(
        read_obj(dir / "nonexistent.obj", dst), std::runtime_error);
}

TEST_F(OBJTest, MTLPresent_NoMapKd_EmptyTexturePaths)
{
    // Write an OBJ that references a MTL but the MTL has no map_Kd
    const auto path     = obj("no_mapkd");
    const auto mtl_path = fs::path(path).replace_extension(".mtl");

    {
        std::ofstream mtl(mtl_path);
        mtl << "newmtl material0\n";
        mtl << "Kd 1.0 1.0 1.0\n";  // diffuse colour but no map_Kd
    }
    {
        // Write OBJ manually referencing the MTL
        std::ofstream f(path);
        f << "mtllib " << mtl_path.filename().string() << "\n";
        f << "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
        f << "vt 0 0\nvt 1 0\nvt 0 1\n";
        f << "usemtl material0\n";
        f << "f 1/1 2/2 3/3\n";
    }

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_obj(path, dst_mesh, dst_uv, dst_textures);

    EXPECT_TRUE(dst_textures.empty());
}

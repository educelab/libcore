#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <sstream>

#include "educelab/core/io/MeshIO.hpp"
#include "educelab/core/io/MeshIO_OBJ.hpp"
#include "educelab/core/io/MeshIO_PLY.hpp"
#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"
#include "educelab/core/utils/MeshUtils.hpp"

namespace fs = std::filesystem;
using namespace educelab;

//------------------------------------------------------------------------------
// Compile-time detection: traits::has_normal<V>
//
// Resolves to true_type when V has a `.normal` member (opt-in via
// traits::WithNormal), false_type otherwise.
//------------------------------------------------------------------------------

// Vertex types under test
using PlainMesh   = Mesh<float, 3>;                            // DefaultVertexTraits
using NormalMesh  = Mesh<float, 3, traits::WithNormal<float, 3>>;
using ColorMesh   = Mesh<float, 3, traits::WithColor>;

static_assert(
    !traits::has_normal<PlainMesh::Vertex>::value,
    "DefaultVertexTraits vertex must not satisfy traits::has_normal");

static_assert(
    traits::has_normal<NormalMesh::Vertex>::value,
    "WithNormal vertex must satisfy traits::has_normal");

static_assert(
    !traits::has_normal<ColorMesh::Vertex>::value,
    "WithColor-only vertex must not satisfy traits::has_normal");

//------------------------------------------------------------------------------
// Compile-time detection: traits::has_color<V>
//
// Resolves to true_type when V has a `.color` member (opt-in via
// traits::WithColor), false_type otherwise.
//------------------------------------------------------------------------------

static_assert(
    !traits::has_color<PlainMesh::Vertex>::value,
    "DefaultVertexTraits vertex must not satisfy traits::has_color");

static_assert(
    !traits::has_color<NormalMesh::Vertex>::value,
    "WithNormal-only vertex must not satisfy traits::has_color");

static_assert(
    traits::has_color<ColorMesh::Vertex>::value,
    "WithColor vertex must satisfy traits::has_color");

//------------------------------------------------------------------------------
// Compile-time detection: traits::has_chart<UVMapT>
//
// Resolves to true_type when UVMapT::Coordinate has a `.chart` member
// (opt-in via traits::WithChart), false_type otherwise.
//------------------------------------------------------------------------------

using PlainUVMap = UVMap<float, 2>;
using ChartUVMap = UVMap<float, 2, traits::WithChart>;

static_assert(
    !traits::has_chart<PlainUVMap>::value,
    "Default UVMap must not satisfy traits::has_chart");

static_assert(
    traits::has_chart<ChartUVMap>::value,
    "UVMap<WithChart> must satisfy traits::has_chart");

//------------------------------------------------------------------------------
// Combined traits: vertex with both normal and color
//------------------------------------------------------------------------------

struct NormalAndColor : traits::WithNormal<float, 3>, traits::WithColor {};

static_assert(
    traits::has_normal<Mesh<float, 3, NormalAndColor>::Vertex>::value,
    "NormalAndColor vertex must satisfy traits::has_normal");

static_assert(
    traits::has_color<Mesh<float, 3, NormalAndColor>::Vertex>::value,
    "NormalAndColor vertex must satisfy traits::has_color");

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
        std::ostringstream ss;
        ss << std::hex << reinterpret_cast<std::uintptr_t>(this);
        dir = fs::temp_directory_path() / ("educelab_meshio_" + ss.str());
        fs::create_directories(dir);
    }

    void TearDown() override { fs::remove_all(dir); }

    [[nodiscard]] auto obj(const std::string& name) const -> fs::path
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
    // v0 = (0,0,0)
    EXPECT_NEAR(dst.vertex(0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(0)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(0)[2], 0.f, 1e-5f);
    // v1 = (1,0,0)
    EXPECT_NEAR(dst.vertex(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[2], 0.f, 1e-5f);
    // v2 = (0,1,0)
    EXPECT_NEAR(dst.vertex(2)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[1], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[2], 0.f, 1e-5f);
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

// A WithNormal mesh whose vertices carry no normals must not emit any vn
// lines (no fabricated "vn 0 0 0") nor normal refs in f lines.
TEST_F(OBJTest, NormalCapableButNoneSet_EmitsNoNormals)
{
    NormalMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.insert_face(0u, 1u, 2u);  // no normals set

    const auto path = obj("no_normals");
    write_obj(path, src);

    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        EXPECT_NE(line.rfind("vn ", 0), 0u)
            << "Unexpected vn line for normal-less mesh: " << line;
        if (line.rfind("f ", 0) == 0) {
            EXPECT_EQ(line.find("//"), std::string::npos)
                << "Unexpected normal ref in face line: " << line;
        }
    }

    NormalMesh dst;
    read_obj(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    EXPECT_FALSE(dst.vertex(0).normal.has_value());
    EXPECT_FALSE(dst.vertex(1).normal.has_value());
    EXPECT_FALSE(dst.vertex(2).normal.has_value());
}

// Only some vertices carry normals: the vn pool is compact (one vn per
// normalled vertex, not per vertex), and f lines reference normals only for
// the corners whose vertex has one.
TEST_F(OBJTest, PartialNormals_CompactPoolAndSelectiveRefs)
{
    NormalMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).normal = Vec3f{0, 0, 1};
    src.vertex(2).normal = Vec3f{1, 0, 0};  // vertex 1 has no normal
    src.insert_face(0u, 1u, 2u);

    const auto path = obj("partial_normals");
    write_obj(path, src);

    std::ifstream f(path);
    std::string line;
    std::size_t vn_count = 0;
    while (std::getline(f, line)) {
        if (line.rfind("vn ", 0) == 0) {
            ++vn_count;
        }
    }
    EXPECT_EQ(vn_count, 2u) << "Compact pool should emit exactly 2 vn lines";

    // Round-trip: v0 -> vn 1, v2 -> vn 2; v1 keeps no normal.
    NormalMesh dst;
    read_obj(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).normal.has_value());
    EXPECT_FALSE(dst.vertex(1).normal.has_value());
    ASSERT_TRUE(dst.vertex(2).normal.has_value());
    EXPECT_NEAR((*dst.vertex(0).normal)[2], 1.f, 1e-5f);
    EXPECT_NEAR((*dst.vertex(2).normal)[0], 1.f, 1e-5f);
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
    ASSERT_TRUE(dst.vertex(1).color.has_value());
    const auto c1 = dst.vertex(1).color.value<Color::F32C3>();
    EXPECT_NEAR(c1[0], 0.f, 1e-5f);
    EXPECT_NEAR(c1[1], 1.f, 1e-5f);
    EXPECT_NEAR(c1[2], 0.f, 1e-5f);
    ASSERT_TRUE(dst.vertex(2).color.has_value());
    const auto c2 = dst.vertex(2).color.value<Color::F32C3>();
    EXPECT_NEAR(c2[0], 0.f, 1e-5f);
    EXPECT_NEAR(c2[1], 0.f, 1e-5f);
    EXPECT_NEAR(c2[2], 1.f, 1e-5f);
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

TEST_F(OBJTest, WriteMissingDir_Throws)
{
    const auto src  = make_triangle();
    EXPECT_THROW(
        write_obj(dir / "nonexistent_subdir" / "out.obj", src),
        std::runtime_error);
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

TEST_F(OBJTest, CombinedNormalsAndUVs_VTVNFormat)
{
    // Build a normal-carrying mesh with UVs so write_obj emits v/vt/vn face refs
    NormalMesh src;
    (void)src.insert_vertex(0.f, 0.f, 0.f);
    (void)src.insert_vertex(1.f, 0.f, 0.f);
    (void)src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).normal = Vec3f{0, 0, 1};
    src.vertex(1).normal = Vec3f{0, 0, 1};
    src.vertex(2).normal = Vec3f{0, 0, 1};
    (void)src.insert_face(0u, 1u, 2u);

    UVMap2f src_uv;
    (void)src_uv.insert(0.f, 0.f);
    (void)src_uv.insert(1.f, 0.f);
    (void)src_uv.insert(0.f, 1.f);
    src_uv.map(0, 0, 0); src_uv.map(0, 1, 1); src_uv.map(0, 2, 2);

    const auto path = obj("vtvn");
    write_obj(path, src, src_uv);

    // Verify file contains v/vt/vn face format
    std::ifstream f(path);
    std::string line;
    bool found_vtvn = false;
    while (std::getline(f, line)) {
        if (line.rfind("f ", 0) == 0 && line.find('/') != std::string::npos) {
            // Check format is v/vt/vn (two slashes per token, no consecutive //)
            const auto tok = split(std::string_view(line));
            if (tok.size() >= 2) {
                const auto ref = std::string(tok[1]);
                const auto first  = ref.find('/');
                const auto second = ref.find('/', first + 1);
                if (first != std::string::npos &&
                    second != std::string::npos &&
                    second > first + 1) {
                    found_vtvn = true;
                }
            }
        }
    }
    EXPECT_TRUE(found_vtvn) << "Expected v/vt/vn face reference format";

    // Round-trip UV and normals
    NormalMesh dst_mesh;
    UVMap2f dst_uv;
    read_obj(path, dst_mesh, dst_uv);
    ASSERT_EQ(dst_mesh.num_vertices(), 3u);
    ASSERT_EQ(dst_uv.size(), 3u);
    ASSERT_TRUE(dst_mesh.vertex(0).normal.has_value());
    EXPECT_NEAR((*dst_mesh.vertex(0).normal)[2], 1.f, 1e-5f);
}

TEST_F(OBJTest, PositionsWithNormalsAndColors)
{
    NCMesh src;
    (void)src.insert_vertex(0.f, 0.f, 0.f);
    (void)src.insert_vertex(1.f, 0.f, 0.f);
    (void)src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).normal = Vec3f{0, 0, 1};
    src.vertex(1).normal = Vec3f{0, 0, 1};
    src.vertex(2).normal = Vec3f{0, 0, 1};
    src.vertex(0).color = Color::F32C3{1.f, 0.f, 0.f};
    src.vertex(1).color = Color::F32C3{0.f, 1.f, 0.f};
    src.vertex(2).color = Color::F32C3{0.f, 0.f, 1.f};
    (void)src.insert_face(0u, 1u, 2u);

    const auto path = obj("nc_mesh");
    write_obj(path, src);

    NCMesh dst;
    read_obj(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).normal.has_value());
    EXPECT_NEAR((*dst.vertex(0).normal)[2], 1.f, 1e-5f);
    ASSERT_TRUE(dst.vertex(0).color.has_value());
    const auto c0 = dst.vertex(0).color.value<Color::F32C3>();
    EXPECT_NEAR(c0[0], 1.f, 1e-5f);
    EXPECT_NEAR(c0[1], 0.f, 1e-5f);
    EXPECT_NEAR(c0[2], 0.f, 1e-5f);
}

//------------------------------------------------------------------------------
// Task 3.1 — expand_at_seams tests
//------------------------------------------------------------------------------

TEST(ExpandAtSeams, NoSeams_NoVertexDuplication)
{
    // Single triangle: every (vertex, uv) pair is unique — no expansion
    const auto mesh = make_triangle();
    const auto uv   = make_triangle_uvmap(mesh);

    const auto [exp, flat] = expand_at_seams(mesh, uv);

    EXPECT_EQ(exp.num_vertices(), 3u);
    EXPECT_EQ(exp.num_faces(), 1u);
    ASSERT_EQ(flat.size(), 3u);

    // Geometry unchanged
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t d = 0; d < 3; ++d) {
            EXPECT_NEAR(exp.vertex(i)[d], mesh.vertex(i)[d], 1e-6f);
        }
    }
}

TEST(ExpandAtSeams, OneSeamEdge_SplitsSeamVertex)
{
    // Two triangles sharing edge (v1, v2): v1 carries different UV in each
    // face → it must be split; v2 has the same UV in both → not split.
    //
    //   v0       v3
    //    |\      /|
    //    | \    / |
    //    |  \  /  |
    //    | f0\/ f1|
    //   v2---v1  (v1 is the seam vertex)
    Mesh3f mesh;
    (void)mesh.insert_vertex(0.f,  0.f, 0.f);  // v0
    (void)mesh.insert_vertex(1.f,  0.f, 0.f);  // v1 — on seam
    (void)mesh.insert_vertex(0.5f, 1.f, 0.f);  // v2 — shared, no seam
    (void)mesh.insert_vertex(2.f,  0.f, 0.f);  // v3
    (void)mesh.insert_face(0u, 1u, 2u);  // face 0
    (void)mesh.insert_face(1u, 2u, 3u);  // face 1

    UVMap2f uv;
    (void)uv.insert(0.f,  0.f);   // pool[0]: v0/face0
    (void)uv.insert(1.f,  0.f);   // pool[1]: v1/face0
    (void)uv.insert(0.5f, 1.f);   // pool[2]: v2 in both faces (same UV)
    (void)uv.insert(0.f,  0.5f);  // pool[3]: v1/face1 ← different → seam!
    (void)uv.insert(1.f,  1.f);   // pool[4]: v3/face1
    uv.map(0, 0, 0);  // face0 corner0 (v0) → pool[0]
    uv.map(0, 1, 1);  // face0 corner1 (v1) → pool[1]
    uv.map(0, 2, 2);  // face0 corner2 (v2) → pool[2]
    uv.map(1, 0, 3);  // face1 corner0 (v1) → pool[3] (seam)
    uv.map(1, 1, 2);  // face1 corner1 (v2) → pool[2] (same — no split)
    uv.map(1, 2, 4);  // face1 corner2 (v3) → pool[4]

    const auto [exp, flat] = expand_at_seams(mesh, uv);

    // v1 was split → 4 original + 1 duplicate = 5
    ASSERT_EQ(exp.num_vertices(), 5u);
    EXPECT_EQ(exp.num_faces(), 2u);
    ASSERT_EQ(flat.size(), 5u);

    // Both faces still have 3 corners
    ASSERT_EQ(exp.face(0).size(), 3u);
    ASSERT_EQ(exp.face(1).size(), 3u);

    // The seam vertex maps to different new indices in each face
    const auto f0_v1_new = exp.face(0)[1];  // v1 as seen by face 0
    const auto f1_v1_new = exp.face(1)[0];  // v1 as seen by face 1
    EXPECT_NE(f0_v1_new, f1_v1_new) << "seam vertex must be split";

    // v2 is not a seam: both faces reference the same expanded vertex
    const auto f0_v2_new = exp.face(0)[2];
    const auto f1_v2_new = exp.face(1)[1];
    EXPECT_EQ(f0_v2_new, f1_v2_new) << "non-seam vertex must not be split";

    // Geometry of both split copies matches original v1 = (1, 0, 0)
    EXPECT_NEAR(exp.vertex(f0_v1_new)[0], 1.f, 1e-6f);
    EXPECT_NEAR(exp.vertex(f0_v1_new)[1], 0.f, 1e-6f);
    EXPECT_NEAR(exp.vertex(f1_v1_new)[0], 1.f, 1e-6f);
    EXPECT_NEAR(exp.vertex(f1_v1_new)[1], 0.f, 1e-6f);
}

TEST(ExpandAtSeams, GeometryIdentical_AllOriginalPositions)
{
    // Four-vertex quad with a UV seam across the diagonal: verify every
    // expanded vertex position is identical to the corresponding original.
    Mesh3f mesh;
    (void)mesh.insert_vertex(0.f, 0.f, 0.f);
    (void)mesh.insert_vertex(1.f, 0.f, 0.f);
    (void)mesh.insert_vertex(1.f, 1.f, 0.f);
    (void)mesh.insert_vertex(0.f, 1.f, 0.f);
    (void)mesh.insert_face(0u, 1u, 2u);
    (void)mesh.insert_face(0u, 2u, 3u);

    // Give v0 and v2 the same UVs in both faces → no seam
    // Give v1 and v3 unique UVs → no seam either
    UVMap2f uv;
    (void)uv.insert(0.f, 0.f);   // pool[0]: v0 in face0
    (void)uv.insert(1.f, 0.f);   // pool[1]: v1
    (void)uv.insert(1.f, 1.f);   // pool[2]: v2 (shared, same pool)
    (void)uv.insert(0.f, 0.f);   // pool[3]: v0 in face1 (same UV → no seam)
    (void)uv.insert(0.f, 1.f);   // pool[4]: v3
    uv.map(0, 0, 0);  uv.map(0, 1, 1);  uv.map(0, 2, 2);
    uv.map(1, 0, 3);  uv.map(1, 1, 2);  uv.map(1, 2, 4);

    const auto [exp, flat] = expand_at_seams(mesh, uv);

    // pool[0] and pool[3] are different indices but identical UVs, so v0 is
    // still split here (different pool indices). That is expected behaviour —
    // expand_at_seams keys on pool index, not UV value.
    EXPECT_EQ(exp.num_faces(), 2u);
    EXPECT_EQ(flat.size(), exp.num_vertices());

    // Every expanded vertex position must match the original vertex it was
    // copied from. We verify by checking face connectivity:
    // face0 → [new(v0), new(v1), new(v2)]
    for (std::size_t fi = 0; fi < exp.num_faces(); ++fi) {
        const auto& orig_face = mesh.face(fi);
        const auto& new_face  = exp.face(fi);
        ASSERT_EQ(orig_face.size(), new_face.size());
        for (std::size_t ci = 0; ci < orig_face.size(); ++ci) {
            const auto orig_vi = orig_face[ci];
            const auto new_vi  = new_face[ci];
            for (std::size_t d = 0; d < 3; ++d) {
                EXPECT_NEAR(
                    exp.vertex(new_vi)[d], mesh.vertex(orig_vi)[d], 1e-6f)
                    << "face " << fi << " corner " << ci << " dim " << d;
            }
        }
    }
}

TEST(ExpandAtSeams, EmptyMesh_ReturnsEmptyExpanded)
{
    Mesh3f mesh;  // zero vertices, zero faces
    UVMap2f uv;

    const auto [exp, flat] = expand_at_seams(mesh, uv);

    EXPECT_EQ(exp.num_vertices(), 0u);
    EXPECT_EQ(exp.num_faces(), 0u);
    EXPECT_TRUE(flat.empty());
}

//------------------------------------------------------------------------------
// PLY Round-Trip Test Fixture
//------------------------------------------------------------------------------

class PLYTest : public ::testing::Test
{
protected:
    fs::path dir;

    void SetUp() override
    {
        std::ostringstream ss;
        ss << std::hex << reinterpret_cast<std::uintptr_t>(this);
        dir = fs::temp_directory_path() / ("educelab_meshio_" + ss.str());
        fs::create_directories(dir);
    }

    void TearDown() override { fs::remove_all(dir); }

    fs::path ply(const std::string& name) const
    {
        return dir / (name + ".ply");
    }
};

//------------------------------------------------------------------------------
// Task 3.3 — PLY Round-Trip Tests
//------------------------------------------------------------------------------

TEST_F(PLYTest, ASCIIPositionsOnly)
{
    const auto src  = make_triangle();
    const auto path = ply("positions");
    write_ply(path, src);

    Mesh3f dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_EQ(dst.num_faces(), 1u);
    // v0 = (0,0,0)
    EXPECT_NEAR(dst.vertex(0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(0)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(0)[2], 0.f, 1e-5f);
    // v1 = (1,0,0)
    EXPECT_NEAR(dst.vertex(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[2], 0.f, 1e-5f);
    // v2 = (0,1,0)
    EXPECT_NEAR(dst.vertex(2)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[1], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[2], 0.f, 1e-5f);
    EXPECT_EQ(dst.face(0), (Mesh3f::Face{0, 1, 2}));
}

TEST_F(PLYTest, ASCIIPositionsWithNormals)
{
    NormalMesh src;
    (void)src.insert_vertex(0.f, 0.f, 0.f);
    (void)src.insert_vertex(1.f, 0.f, 0.f);
    (void)src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).normal = Vec3f{0, 0, 1};
    src.vertex(1).normal = Vec3f{0, 0, 1};
    src.vertex(2).normal = Vec3f{0, 0, 1};
    (void)src.insert_face(0u, 1u, 2u);

    const auto path = ply("normals");
    write_ply(path, src);

    NormalMesh dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).normal.has_value());
    EXPECT_NEAR((*dst.vertex(0).normal)[2], 1.f, 1e-5f);
    EXPECT_NEAR((*dst.vertex(2).normal)[2], 1.f, 1e-5f);
}

TEST_F(PLYTest, ASCIIPositionsWithColors)
{
    ColorMesh src;
    (void)src.insert_vertex(0.f, 0.f, 0.f);
    (void)src.insert_vertex(1.f, 0.f, 0.f);
    (void)src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).color = Color::U8C3{255, 0, 0};
    src.vertex(1).color = Color::U8C3{0, 255, 0};
    src.vertex(2).color = Color::U8C3{0, 0, 255};
    (void)src.insert_face(0u, 1u, 2u);

    const auto path = ply("colors");
    write_ply(path, src);

    ColorMesh dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).color.has_value());
    const auto c0 = dst.vertex(0).color.value<Color::U8C3>();
    EXPECT_EQ(c0[0], 255u);
    EXPECT_EQ(c0[1], 0u);
    EXPECT_EQ(c0[2], 0u);
    const auto c1 = dst.vertex(1).color.value<Color::U8C3>();
    EXPECT_EQ(c1[0], 0u);
    EXPECT_EQ(c1[1], 255u);
    EXPECT_EQ(c1[2], 0u);
    ASSERT_TRUE(dst.vertex(2).color.has_value());
    const auto c2 = dst.vertex(2).color.value<Color::U8C3>();
    EXPECT_EQ(c2[0], 0u);
    EXPECT_EQ(c2[1], 0u);
    EXPECT_EQ(c2[2], 255u);
}

TEST_F(PLYTest, NGonFace_Quad)
{
    const auto src  = make_quad();
    const auto path = ply("quad");
    write_ply(path, src);

    Mesh3f dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 4u);
    ASSERT_EQ(dst.num_faces(), 1u);
    EXPECT_EQ(dst.face(0).size(), 4u);
}

TEST_F(PLYTest, BinaryLittleEndian_Read)
{
    // Hand-craft a binary-little-endian PLY with one triangle
    const auto path = ply("binary");
    {
        std::ofstream f(path, std::ios::binary);
        // ASCII header
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element vertex 3\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 1\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        // Vertex data (3 × 3 floats, little-endian)
        const float verts[9] = {
            0.f, 0.f, 0.f,
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
        // Face data: count=3, indices 0 1 2
        const uint8_t  cnt = 3;
        const int32_t  idx[3] = {0, 1, 2};
        f.write(reinterpret_cast<const char*>(&cnt), 1);
        f.write(reinterpret_cast<const char*>(idx), sizeof(idx));
    }

    Mesh3f dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_EQ(dst.num_faces(), 1u);
    // v0 = (0,0,0)
    EXPECT_NEAR(dst.vertex(0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(0)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(0)[2], 0.f, 1e-5f);
    // v1 = (1,0,0)
    EXPECT_NEAR(dst.vertex(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(1)[2], 0.f, 1e-5f);
    // v2 = (0,1,0)
    EXPECT_NEAR(dst.vertex(2)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[1], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[2], 0.f, 1e-5f);
    EXPECT_EQ(dst.face(0), (Mesh3f::Face{0, 1, 2}));
}

TEST_F(PLYTest, WriteWithUVMap_PerWedgeTexcoord)
{
    // Two triangles sharing an edge with a UV seam.
    // The texcoord approach stores per-wedge UVs in the face element, so no
    // vertex duplication is needed — vertex count stays at 4.
    Mesh3f src_mesh;
    (void)src_mesh.insert_vertex(0.f,  0.f, 0.f);  // v0
    (void)src_mesh.insert_vertex(1.f,  0.f, 0.f);  // v1 — seam vertex
    (void)src_mesh.insert_vertex(0.5f, 1.f, 0.f);  // v2 — shared, no seam
    (void)src_mesh.insert_vertex(2.f,  0.f, 0.f);  // v3
    (void)src_mesh.insert_face(0u, 1u, 2u);
    (void)src_mesh.insert_face(1u, 2u, 3u);

    UVMap2f src_uv;
    (void)src_uv.insert(0.f,  0.f);   // pool[0]: v0
    (void)src_uv.insert(1.f,  0.f);   // pool[1]: v1/face0
    (void)src_uv.insert(0.5f, 1.f);   // pool[2]: v2 (both faces)
    (void)src_uv.insert(0.f,  0.5f);  // pool[3]: v1/face1 ← seam
    (void)src_uv.insert(1.f,  1.f);   // pool[4]: v3
    src_uv.map(0, 0, 0);  src_uv.map(0, 1, 1);  src_uv.map(0, 2, 2);
    src_uv.map(1, 0, 3);  src_uv.map(1, 1, 2);  src_uv.map(1, 2, 4);

    const auto path = ply("wedge_uv");
    write_ply(path, src_mesh, src_uv);

    // Read back: no seam expansion — geometry is unchanged
    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_ply(path, dst_mesh, dst_uv, dst_textures);

    EXPECT_EQ(dst_mesh.num_vertices(), 4u);
    EXPECT_EQ(dst_mesh.num_faces(), 2u);
    // 6 pool entries: one per face-corner (3 corners × 2 faces)
    EXPECT_EQ(dst_uv.size(), 6u);
    EXPECT_TRUE(dst_textures.empty());

    // Verify per-wedge UV values round-trip correctly
    EXPECT_NEAR(
        dst_uv.get_coordinate(0, 0)[0], 0.f, 1e-5f);  // face0 corner0 → (0,0)
    EXPECT_NEAR(dst_uv.get_coordinate(0, 0)[1], 0.f, 1e-5f);
    EXPECT_NEAR(
        dst_uv.get_coordinate(0, 1)[0], 1.f, 1e-5f);  // face0 corner1 → (1,0)
    EXPECT_NEAR(dst_uv.get_coordinate(0, 1)[1], 0.f, 1e-5f);
    // face1 corner0 maps to pool[3]=(0,0.5) — seam: same vertex as
    // face0-corner1 but different UV
    EXPECT_NEAR(dst_uv.get_coordinate(1, 0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst_uv.get_coordinate(1, 0)[1], 0.5f, 1e-5f);
}

TEST_F(PLYTest, ReadLegacyPerVertexUV_BackwardCompat)
{
    // Hand-craft a PLY that uses the old per-vertex s/t scalar approach.
    // read_ply should fall back to the s/t path and populate the uvmap.
    const auto path = ply("legacy_st");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 3\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "property float s\n"
          << "property float t\n"
          << "element face 1\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n"
          << "0 0 0 0.0 0.0\n"
          << "1 0 0 1.0 0.0\n"
          << "0 1 0 0.0 1.0\n"
          << "3 0 1 2\n";
    }

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    read_ply(path, dst_mesh, dst_uv);

    ASSERT_EQ(dst_mesh.num_vertices(), 3u);
    ASSERT_EQ(dst_mesh.num_faces(), 1u);
    // One pool entry per vertex; pool index == vertex index
    ASSERT_EQ(dst_uv.size(), 3u);

    // Verify UV values and wedge mappings
    EXPECT_NEAR(dst_uv.at(0)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(0)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(1)[1], 0.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(2)[0], 0.f, 1e-5f);
    EXPECT_NEAR(dst_uv.at(2)[1], 1.f, 1e-5f);
    // Face 0 corners map to pool entries 0, 1, 2 respectively
    EXPECT_EQ(dst_uv.get(0, 0), 0u);
    EXPECT_EQ(dst_uv.get(0, 1), 1u);
    EXPECT_EQ(dst_uv.get(0, 2), 2u);
}

TEST_F(PLYTest, WriteWithUVMap_TexturePath_CommentInHeader)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto tex_path = fs::path("texture.png");
    const auto path     = ply("tex");
    write_ply(path, src_mesh, src_uv, tex_path);

    // Verify header contains "comment TextureFile texture.png"
    std::ifstream f(path);
    std::string line;
    bool found_comment = false;
    while (std::getline(f, line)) {
        if (line == "end_header") break;
        if (line.rfind("comment TextureFile", 0) == 0) {
            found_comment = true;
            EXPECT_NE(line.find("texture.png"), std::string::npos);
        }
    }
    EXPECT_TRUE(found_comment) << "Expected 'comment TextureFile' in PLY header";

    // Round-trip: texture_path recovered
    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_ply(path, dst_mesh, dst_uv, dst_textures);

    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_EQ(dst_textures[0], tex_path);
}

TEST_F(PLYTest, ReadCommentTextureFile_HandCraftedPLY)
{
    // Hand-craft a PLY with two texture comments
    const auto path = ply("two_textures");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "comment TextureFile atlas0.png\n"
          << "comment TextureFile atlas1.png\n"
          << "element vertex 3\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 1\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n"
          << "0 0 0\n"
          << "1 0 0\n"
          << "0 1 0\n"
          << "3 0 1 2\n";
    }

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_ply(path, dst_mesh, dst_uv, dst_textures);

    ASSERT_EQ(dst_textures.size(), 2u);
    EXPECT_EQ(dst_textures[0], fs::path("atlas0.png"));
    EXPECT_EQ(dst_textures[1], fs::path("atlas1.png"));
}

TEST_F(PLYTest, ReadMissingFile_Throws)
{
    Mesh3f dst;
    EXPECT_THROW(
        read_ply(dir / "nonexistent.ply", dst), std::runtime_error);
}

TEST_F(PLYTest, WriteMissingDir_Throws)
{
    const auto src  = make_triangle();
    EXPECT_THROW(
        write_ply(dir / "nonexistent_subdir" / "out.ply", src),
        std::runtime_error);
}

TEST_F(PLYTest, MalformedHeader_Throws)
{
    const auto path = ply("bad_header");
    {
        std::ofstream f(path);
        f << "not_a_ply_file\nelement vertex 3\n";
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

TEST_F(PLYTest, BinaryBigEndian_Throws)
{
    const auto path = ply("big_endian");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_big_endian 1.0\n"
          << "element vertex 3\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 1\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        // Some placeholder data (will never be read)
        const float verts[9] = {};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

//------------------------------------------------------------------------------
// Task 4.1 — read_mesh / write_mesh convenience facade tests
//------------------------------------------------------------------------------

class MeshIOTest : public ::testing::Test
{
protected:
    fs::path dir;

    void SetUp() override
    {
        std::ostringstream ss;
        ss << std::hex << reinterpret_cast<std::uintptr_t>(this);
        dir = fs::temp_directory_path() / ("educelab_meshio_" + ss.str());
        fs::create_directories(dir);
    }

    void TearDown() override { fs::remove_all(dir); }
};

TEST_F(MeshIOTest, OBJExtension_DispatchesToOBJ)
{
    const auto src  = make_triangle();
    const auto path = dir / "dispatch.obj";
    write_mesh(path, src);
    ASSERT_TRUE(fs::exists(path));

    Mesh3f dst;
    read_mesh(path, dst);
    EXPECT_EQ(dst.num_vertices(), 3u);
    EXPECT_EQ(dst.num_faces(), 1u);
}

TEST_F(MeshIOTest, PLYExtension_DispatchesToPLY)
{
    const auto src  = make_triangle();
    const auto path = dir / "dispatch.ply";
    write_mesh(path, src);
    ASSERT_TRUE(fs::exists(path));

    Mesh3f dst;
    read_mesh(path, dst);
    EXPECT_EQ(dst.num_vertices(), 3u);
    EXPECT_EQ(dst.num_faces(), 1u);
}

TEST_F(MeshIOTest, UnsupportedExtension_Throws)
{
    const auto src  = make_triangle();
    const auto path = dir / "model.stl";
    EXPECT_THROW(write_mesh(path, src), std::runtime_error);

    Mesh3f dst;
    EXPECT_THROW(read_mesh(path, dst), std::runtime_error);
}

TEST_F(MeshIOTest, WriteMissingDir_Throws)
{
    const auto src  = make_triangle();
    EXPECT_THROW(
        write_mesh(dir / "nonexistent_subdir" / "out.obj", src),
        std::runtime_error);
}

TEST_F(MeshIOTest, OBJ_WithUVMap_DispatchesTier2)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto path     = dir / "uv.obj";
    write_mesh(path, src_mesh, src_uv);

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    read_mesh(path, dst_mesh, dst_uv);
    EXPECT_EQ(dst_uv.size(), 3u);
}

TEST_F(MeshIOTest, PLY_WithUVMap_DispatchesTier2)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto path     = dir / "uv.ply";
    write_mesh(path, src_mesh, src_uv);

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_mesh(path, dst_mesh, dst_uv, dst_textures);
    EXPECT_EQ(dst_uv.size(), 3u);
    EXPECT_TRUE(dst_textures.empty());
}

TEST_F(MeshIOTest, OBJ_WithUVMapAndTexturePath_DispatchesTier3)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto tex      = fs::path("texture.png");
    const auto path     = dir / "tex.obj";
    write_mesh(path, src_mesh, src_uv, tex);
    ASSERT_TRUE(fs::exists(fs::path(path).replace_extension(".mtl")));

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_mesh(path, dst_mesh, dst_uv, dst_textures);
    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_EQ(dst_textures[0], tex);
}

TEST_F(MeshIOTest, PLY_WithUVMapAndTexturePath_DispatchesTier3)
{
    const auto src_mesh = make_triangle();
    const auto src_uv   = make_triangle_uvmap(src_mesh);
    const auto tex      = fs::path("texture.png");
    const auto path     = dir / "tex.ply";
    write_mesh(path, src_mesh, src_uv, tex);

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_mesh(path, dst_mesh, dst_uv, dst_textures);
    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_EQ(dst_textures[0], tex);
}

//------------------------------------------------------------------------------
// Edge-case tests — robustness and error handling
//------------------------------------------------------------------------------

// OBJ: face referencing a vertex index that doesn't exist should throw
TEST_F(OBJTest, FaceWithOutOfBoundsVertexIndex_Throws)
{
    const auto path = obj("bad_face");
    {
        std::ofstream f(path);
        f << "v 0 0 0\n"
          << "v 1 0 0\n"
          << "v 0 1 0\n"
          << "f 1 2 99\n";  // vertex 99 does not exist
    }
    Mesh3f dst;
    EXPECT_THROW(read_obj(path, dst), std::runtime_error);
}

// PLY: truncated binary data should throw instead of silently producing garbage
TEST_F(PLYTest, TruncatedBinaryData_Throws)
{
    const auto path = ply("truncated");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element vertex 3\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 1\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        // Only write 1 vertex instead of 3 (truncated)
        const float verts[3] = {0.f, 0.f, 0.f};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

// PLY: header claiming unreasonably large element count should throw
TEST_F(PLYTest, ExcessiveElementCount_Throws)
{
    const auto path = ply("excessive");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 999999999999\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

// MeshIO: case-insensitive extension dispatch
TEST_F(MeshIOTest, UppercaseExtension_DispatchesCorrectly)
{
    const auto src  = make_triangle();
    // Write with lowercase, read with uppercase path
    const auto path_lower = dir / "model.obj";
    write_mesh(path_lower, src);

    // Rename to uppercase
    const auto path_upper = dir / "model.OBJ";
    fs::rename(path_lower, path_upper);

    Mesh3f dst;
    read_mesh(path_upper, dst);
    EXPECT_EQ(dst.num_vertices(), 3u);
    EXPECT_EQ(dst.num_faces(), 1u);
}

// MeshIO: mixed-case extension dispatch
TEST_F(MeshIOTest, MixedCaseExtension_DispatchesCorrectly)
{
    const auto src  = make_triangle();
    const auto path_lower = dir / "model.ply";
    write_mesh(path_lower, src);

    const auto path_mixed = dir / "model.Ply";
    fs::rename(path_lower, path_mixed);

    Mesh3f dst;
    read_mesh(path_mixed, dst);
    EXPECT_EQ(dst.num_vertices(), 3u);
    EXPECT_EQ(dst.num_faces(), 1u);
}

// OBJ: empty mesh round-trip
TEST_F(OBJTest, EmptyMesh_RoundTrip)
{
    const auto path = obj("empty");
    Mesh3f src;
    write_obj(path, src);

    Mesh3f dst;
    read_obj(path, dst);
    EXPECT_EQ(dst.num_vertices(), 0u);
    EXPECT_EQ(dst.num_faces(), 0u);
}

// PLY: empty mesh round-trip
TEST_F(PLYTest, EmptyMesh_RoundTrip)
{
    const auto path = ply("empty");
    Mesh3f src;
    write_ply(path, src);

    Mesh3f dst;
    read_ply(path, dst);
    EXPECT_EQ(dst.num_vertices(), 0u);
    EXPECT_EQ(dst.num_faces(), 0u);
}

// PLY: malformed "element" line (missing count) should throw
TEST_F(PLYTest, MalformedElementLine_Throws)
{
    const auto path = ply("malformed_element");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex\n"  // missing count
          << "property float x\n"
          << "end_header\n";
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

// PLY: "property" line before any "element" should throw
TEST_F(PLYTest, PropertyBeforeElement_Throws)
{
    const auto path = ply("property_no_element");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "property float x\n"  // no preceding element
          << "end_header\n";
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

// PLY: malformed "property" line (too few tokens) should throw
TEST_F(PLYTest, MalformedPropertyLine_Throws)
{
    const auto path = ply("malformed_property");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 1\n"
          << "property float\n"  // missing name
          << "end_header\n";
    }
    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

// PLY: face record with texcoord list count beyond the per-face safety cap
// should throw rather than attempting to allocate an unbounded buffer.
TEST_F(PLYTest, ExcessiveTexcoordCount_Throws)
{
    const auto path = ply("excessive_texcoord");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 3\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 1\n"
          << "property list uchar int vertex_indices\n"
          << "property list uint float texcoord\n"
          << "end_header\n"
          << "0 0 0\n"
          << "1 0 0\n"
          << "0 1 0\n"
          // 3 corners, then a bogus texcoord count of ~4 billion
          << "3 0 1 2 4294967295\n";
    }
    Mesh3f dst;
    UVMap2f uv;
    EXPECT_THROW(read_ply(path, dst, uv), std::runtime_error);
}

// PLY: vertex colors declared as float are stored as F32C3, not truncated to
// uint8. Values in [0, 1] would otherwise collapse to 0 under the old cast.
TEST_F(PLYTest, ReadFloatColor_StoresAsF32C3)
{
    const auto path = ply("float_color");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 1\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "property float red\n"
          << "property float green\n"
          << "property float blue\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n"
          << "0 0 0 0.25 0.5 0.75\n";
    }
    Mesh<float, 3, traits::WithColor> dst;
    read_ply(path, dst);
    ASSERT_EQ(dst.num_vertices(), 1u);
    const auto& c = dst.vertex(0).color;
    ASSERT_EQ(c.type(), Color::Type::F32C3);
    const auto rgb = c.value<Color::F32C3>();
    EXPECT_FLOAT_EQ(rgb[0], 0.25f);
    EXPECT_FLOAT_EQ(rgb[1], 0.50f);
    EXPECT_FLOAT_EQ(rgb[2], 0.75f);
}

// PLY: vertex colors declared as ushort are stored as U16C3, not narrowed to
// uint8. Values above 255 would otherwise wrap under the old cast.
TEST_F(PLYTest, ReadUShortColor_StoresAsU16C3)
{
    const auto path = ply("ushort_color");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 1\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "property ushort red\n"
          << "property ushort green\n"
          << "property ushort blue\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n"
          << "0 0 0 1000 20000 65535\n";
    }
    Mesh<float, 3, traits::WithColor> dst;
    read_ply(path, dst);
    ASSERT_EQ(dst.num_vertices(), 1u);
    const auto& c = dst.vertex(0).color;
    ASSERT_EQ(c.type(), Color::Type::U16C3);
    const auto rgb = c.value<Color::U16C3>();
    EXPECT_EQ(rgb[0], 1000u);
    EXPECT_EQ(rgb[1], 20000u);
    EXPECT_EQ(rgb[2], 65535u);
}

// PLY: vertex colors declared as uchar remain stored as U8C3 (default path)
TEST_F(PLYTest, ReadUCharColor_StoresAsU8C3)
{
    const auto path = ply("uchar_color");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 1\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "property uchar red\n"
          << "property uchar green\n"
          << "property uchar blue\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n"
          << "0 0 0 10 128 250\n";
    }
    Mesh<float, 3, traits::WithColor> dst;
    read_ply(path, dst);
    ASSERT_EQ(dst.num_vertices(), 1u);
    const auto& c = dst.vertex(0).color;
    ASSERT_EQ(c.type(), Color::Type::U8C3);
    const auto rgb = c.value<Color::U8C3>();
    EXPECT_EQ(rgb[0], 10u);
    EXPECT_EQ(rgb[1], 128u);
    EXPECT_EQ(rgb[2], 250u);
}

// OBJ: "mtllib" directive with no filename argument should throw
TEST_F(OBJTest, MtllibWithoutArgument_Throws)
{
    const auto path = obj("bare_mtllib");
    {
        std::ofstream f(path);
        f << "mtllib\n"
          << "v 0 0 0\n"
          << "v 1 0 0\n"
          << "v 0 1 0\n"
          << "f 1 2 3\n";
    }
    Mesh3f dst;
    UVMap2f uv;
    std::vector<fs::path> tex;
    EXPECT_THROW(read_obj(path, dst, uv, tex), std::runtime_error);
}

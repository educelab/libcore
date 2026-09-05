#include <gtest/gtest.h>

#include <array>
#include <csignal>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <sstream>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/resource.h>
#endif

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

// A WithColor mesh whose vertices carry no colors must not append inline RGB
// (no fabricated black); v lines stay "v x y z" and round-trip color-less.
TEST_F(OBJTest, ColorCapableButNoneSet_EmitsNoColors)
{
    ColorMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.insert_face(0u, 1u, 2u);  // no colors set

    const auto path = obj("no_colors");
    write_obj(path, src);

    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        if (line.rfind("v ", 0) == 0) {
            const auto toks = split(std::string_view(line));
            EXPECT_EQ(toks.size(), 4u)
                << "Color-less vertex should emit 'v x y z' only: " << line;
        }
    }

    ColorMesh dst;
    read_obj(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    EXPECT_FALSE(dst.vertex(0).color.has_value());
    EXPECT_FALSE(dst.vertex(1).color.has_value());
    EXPECT_FALSE(dst.vertex(2).color.has_value());
}

// Only some vertices carry colors: OBJ emits inline RGB per vertex, so the
// color-less corner round-trips with no color while the others keep theirs.
TEST_F(OBJTest, PartialColors_PerVertexInlineRGB)
{
    ColorMesh src;
    src.insert_vertex(0.f, 0.f, 0.f);
    src.insert_vertex(1.f, 0.f, 0.f);
    src.insert_vertex(0.f, 1.f, 0.f);
    src.vertex(0).color = Color::F32C3{1.f, 0.f, 0.f};
    src.vertex(2).color = Color::F32C3{0.f, 0.f, 1.f};  // vertex 1 uncolored
    src.insert_face(0u, 1u, 2u);

    const auto path = obj("partial_colors");
    write_obj(path, src);

    ColorMesh dst;
    read_obj(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_TRUE(dst.vertex(0).color.has_value());
    EXPECT_FALSE(dst.vertex(1).color.has_value());
    ASSERT_TRUE(dst.vertex(2).color.has_value());
    const auto c0 = dst.vertex(0).color.value<Color::F32C3>();
    EXPECT_NEAR(c0[0], 1.f, 1e-5f);
    const auto c2 = dst.vertex(2).color.value<Color::F32C3>();
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

    // Walk the file tracking which usemtl each face falls under. It is not
    // enough to count the directives: we must confirm faces are *grouped*
    // under the correct material. Chart 0's UVs occupy vt pool 1..3 (1-based)
    // and chart 1's occupy vt pool 4..6, so the vt index on each face corner
    // tells us which chart that face belongs to.
    std::ifstream f(path);
    std::string line;
    std::vector<std::string> usemtls;        // directives, in file order
    std::string current_mtl;                 // material in effect for faces
    // For each material, the set of vt pool indices (1-based) its faces use.
    std::vector<std::pair<std::string, std::vector<std::size_t>>> faces_under;
    while (std::getline(f, line)) {
        if (line.rfind("usemtl", 0) == 0) {
            const auto toks = split(std::string_view(line));
            ASSERT_GE(toks.size(), 2u) << "usemtl missing material name";
            current_mtl = std::string(toks[1]);
            usemtls.push_back(current_mtl);
        } else if (line.rfind("f ", 0) == 0) {
            EXPECT_FALSE(current_mtl.empty())
                << "face emitted before any usemtl: " << line;
            const auto toks = split(std::string_view(line));
            std::vector<std::size_t> vts;
            for (std::size_t i = 1; i < toks.size(); ++i) {
                const auto ref   = std::string(toks[i]);
                const auto slash = ref.find('/');
                ASSERT_NE(slash, std::string::npos)
                    << "face corner missing vt ref: " << line;
                vts.push_back(std::stoul(ref.substr(slash + 1)));
            }
            faces_under.emplace_back(current_mtl, std::move(vts));
        }
    }

    // Exactly two materials, declared in chart-index order.
    ASSERT_EQ(usemtls.size(), 2u);
    EXPECT_EQ(usemtls[0], "material0");
    EXPECT_EQ(usemtls[1], "material1");

    // Exactly two faces, each grouped under the material for its chart:
    // face on chart 0 → material0 with vt refs in 1..3; chart 1 → material1
    // with vt refs in 4..6.
    ASSERT_EQ(faces_under.size(), 2u);
    for (const auto& [mtl, vts] : faces_under) {
        ASSERT_EQ(vts.size(), 3u);
        if (mtl == "material0") {
            for (const auto vt : vts) {
                EXPECT_GE(vt, 1u);
                EXPECT_LE(vt, 3u) << "chart-0 face references chart-1 vt " << vt;
            }
        } else if (mtl == "material1") {
            for (const auto vt : vts) {
                EXPECT_GE(vt, 4u) << "chart-1 face references chart-0 vt " << vt;
                EXPECT_LE(vt, 6u);
            }
        } else {
            ADD_FAILURE() << "face grouped under unexpected material: " << mtl;
        }
    }

    // Round-trip into a WithChart UVMap
    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> dst_textures;
    read_obj(path, dst_mesh, dst_uv, dst_textures);

    ASSERT_EQ(dst_textures.size(), 2u);
    EXPECT_EQ(dst_textures[0], tex_paths[0]);
    EXPECT_EQ(dst_textures[1], tex_paths[1]);
    // Chart indices recovered for every corner of both faces, not just corner 0.
    for (std::size_t corner = 0; corner < 3; ++corner) {
        EXPECT_EQ(dst_uv.at(dst_uv.get(0, corner)).chart, 0u)
            << "face 0 corner " << corner << " should be on chart 0";
        EXPECT_EQ(dst_uv.at(dst_uv.get(1, corner)).chart, 1u)
            << "face 1 corner " << corner << " should be on chart 1";
    }
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

// Writes three single-triangle charts to `path`, using materials in usemtl
// usage order material_00, material_01, material_02. Each face gets its own
// three vt entries so chart indices are assigned per-pool-index cleanly.
static void write_three_chart_obj(const fs::path& path, const std::string& mtl)
{
    std::ofstream o(path);
    o << "mtllib " << mtl << '\n';
    o << "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
    o << "v 2 0 0\nv 3 0 0\nv 2 1 0\n";
    o << "v 4 0 0\nv 5 0 0\nv 4 1 0\n";
    o << "vt 0.0 0.0\nvt 0.1 0.0\nvt 0.0 0.1\n";
    o << "vt 0.2 0.0\nvt 0.3 0.0\nvt 0.2 0.1\n";
    o << "vt 0.4 0.0\nvt 0.5 0.0\nvt 0.4 0.1\n";
    o << "usemtl material_00\nf 1/1 2/2 3/3\n";
    o << "usemtl material_01\nf 4/4 5/5 6/6\n";
    o << "usemtl material_02\nf 7/7 8/8 9/9\n";
}

// The .mtl declares materials in a different order than the .obj uses them.
// Chart indices follow usemtl *usage* order; texture_paths must be aligned to
// chart index by material *name*, not by .mtl declaration order.
TEST_F(OBJTest, ReadMultiChart_TexturePathsIndexedByChartNotMtlOrder)
{
    {
        std::ofstream mtl(dir / "reorder.mtl");
        mtl << "newmtl material_00\nmap_Kd 00.jpg\n"   // decl 0
            << "newmtl material_02\nmap_Kd 02.jpg\n"   // decl 1 (out of order)
            << "newmtl material_01\nmap_Kd 01.jpg\n";  // decl 2
    }
    const auto path = obj("reorder");
    write_three_chart_obj(path, "reorder.mtl");

    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> tex;
    read_obj(path, dst_mesh, dst_uv, tex);

    ASSERT_EQ(dst_mesh.num_faces(), 3u);
    ASSERT_EQ(tex.size(), 3u);
    // Face fi uses material_0{fi}, whose map_Kd is 0{fi}.jpg. Whatever chart
    // index that face resolved to, texture_paths[chart] must be its own image.
    const std::array<fs::path, 3> expected{
        fs::path("00.jpg"), fs::path("01.jpg"), fs::path("02.jpg")};
    for (std::size_t fi = 0; fi < 3; ++fi) {
        const auto chart = dst_uv.at(dst_uv.get(fi, 0)).chart;
        EXPECT_EQ(tex[chart], expected[fi])
            << "face " << fi << " (chart " << chart << ") got wrong texture";
    }
}

// A used material with no map_Kd must keep an empty texture_paths slot (not be
// compacted out), so later charts retain their correct images.
TEST_F(OBJTest, ReadMultiChart_MaterialWithoutMapKd_PreservesEmptySlot)
{
    {
        std::ofstream mtl(dir / "missing.mtl");
        mtl << "newmtl material_00\nmap_Kd 00.jpg\n"
            << "newmtl material_01\n"                  // no map_Kd
            << "newmtl material_02\nmap_Kd 02.jpg\n";
    }
    const auto path = obj("missing");
    write_three_chart_obj(path, "missing.mtl");

    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> tex;
    read_obj(path, dst_mesh, dst_uv, tex);

    ASSERT_EQ(tex.size(), 3u);
    // Charts follow usage order 00, 01, 02 → 0, 1, 2.
    const auto chart0 = dst_uv.at(dst_uv.get(0, 0)).chart;
    const auto chart1 = dst_uv.at(dst_uv.get(1, 0)).chart;
    const auto chart2 = dst_uv.at(dst_uv.get(2, 0)).chart;
    EXPECT_EQ(tex[chart0], fs::path("00.jpg"));
    EXPECT_TRUE(tex[chart1].empty())
        << "material with no map_Kd should keep an empty slot, got "
        << tex[chart1];
    EXPECT_EQ(tex[chart2], fs::path("02.jpg"));
}

// A material re-entered later in the OBJ (usemtl A ... B ... A) must resolve
// to the same chart/texture on both uses; chart indexing is by name, so no new
// chart is created on re-entry.
TEST_F(OBJTest, ReadMultiChart_MaterialReEntry_ReusesSameChart)
{
    {
        std::ofstream mtl(dir / "reentry.mtl");
        mtl << "newmtl material_00\nmap_Kd a.jpg\n"
            << "newmtl material_01\nmap_Kd b.jpg\n";
    }
    const auto path = obj("reentry");
    {
        std::ofstream o(path);
        o << "mtllib reentry.mtl\n";
        o << "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
        o << "v 2 0 0\nv 3 0 0\nv 2 1 0\n";
        o << "v 4 0 0\nv 5 0 0\nv 4 1 0\n";
        o << "vt 0.0 0.0\nvt 0.1 0.0\nvt 0.0 0.1\n";
        o << "vt 0.2 0.0\nvt 0.3 0.0\nvt 0.2 0.1\n";
        o << "vt 0.4 0.0\nvt 0.5 0.0\nvt 0.4 0.1\n";
        o << "usemtl material_00\nf 1/1 2/2 3/3\n";   // chart for material_00
        o << "usemtl material_01\nf 4/4 5/5 6/6\n";   // chart for material_01
        o << "usemtl material_00\nf 7/7 8/8 9/9\n";   // re-entry of material_00
    }

    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> tex;
    read_obj(path, dst_mesh, dst_uv, tex);

    ASSERT_EQ(dst_mesh.num_faces(), 3u);
    // Exactly two charts: material_00 and material_01. The re-entry does not
    // create a third.
    ASSERT_EQ(tex.size(), 2u);
    const auto chart_f0 = dst_uv.at(dst_uv.get(0, 0)).chart;
    const auto chart_f1 = dst_uv.at(dst_uv.get(1, 0)).chart;
    const auto chart_f2 = dst_uv.at(dst_uv.get(2, 0)).chart;
    EXPECT_EQ(chart_f0, chart_f2) << "re-entered material must reuse its chart";
    EXPECT_NE(chart_f0, chart_f1);
    EXPECT_EQ(tex[chart_f0], fs::path("a.jpg"));
    EXPECT_EQ(tex[chart_f1], fs::path("b.jpg"));
}

// With no usemtl directives, texture_paths falls back to emitting each map_Kd
// in .mtl declaration order (legacy single-/implicit-material behavior).
TEST_F(OBJTest, ReadNoUsemtl_TexturePathsInDeclarationOrder)
{
    {
        std::ofstream mtl(dir / "legacy.mtl");
        mtl << "newmtl material_00\nmap_Kd first.jpg\n"
            << "newmtl material_01\nmap_Kd second.jpg\n";
    }
    const auto path = obj("legacy");
    {
        std::ofstream o(path);
        o << "mtllib legacy.mtl\n";
        o << "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
        o << "vt 0 0\nvt 1 0\nvt 0 1\n";
        o << "f 1/1 2/2 3/3\n";  // no usemtl
    }

    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> tex;
    read_obj(path, dst_mesh, dst_uv, tex);

    ASSERT_EQ(tex.size(), 2u);
    EXPECT_EQ(tex[0], fs::path("first.jpg"));
    EXPECT_EQ(tex[1], fs::path("second.jpg"));
}

// map_Kd captures the rest of the line, so texture filenames with spaces
// survive the round-trip (previously truncated at the first token).
TEST_F(OBJTest, ReadMapKd_FilenameWithSpaces)
{
    {
        std::ofstream mtl(dir / "spaces.mtl");
        mtl << "newmtl material_00\nmap_Kd my texture file.jpg\n";
    }
    const auto path = obj("spaces");
    {
        std::ofstream o(path);
        o << "mtllib spaces.mtl\n";
        o << "v 0 0 0\nv 1 0 0\nv 0 1 0\n";
        o << "vt 0 0\nvt 1 0\nvt 0 1\n";
        o << "usemtl material_00\nf 1/1 2/2 3/3\n";
    }

    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> tex;
    read_obj(path, dst_mesh, dst_uv, tex);

    ASSERT_EQ(tex.size(), 1u);
    EXPECT_EQ(tex[0], fs::path("my texture file.jpg"));
}

// CRLF line endings must not leak a trailing '\r' into the resolved mtllib
// path; otherwise the MTL fails to open and texture_paths comes back empty.
TEST_F(OBJTest, ReadMtllib_CRLFLineEndings)
{
    {
        std::ofstream mtl(dir / "crlf.mtl");
        mtl << "newmtl material_00\nmap_Kd tex.jpg\n";
    }
    const auto path = obj("crlf");
    {
        std::ofstream o(path, std::ios::binary);  // preserve CRLF
        o << "mtllib crlf.mtl\r\n";
        o << "v 0 0 0\r\nv 1 0 0\r\nv 0 1 0\r\n";
        o << "vt 0 0\r\nvt 1 0\r\nvt 0 1\r\n";
        o << "usemtl material_00\r\nf 1/1 2/2 3/3\r\n";
    }

    Mesh3f dst_mesh;
    ChartUVMap dst_uv;
    std::vector<fs::path> tex;
    read_obj(path, dst_mesh, dst_uv, tex);

    ASSERT_EQ(tex.size(), 1u);
    EXPECT_EQ(tex[0], fs::path("tex.jpg"))
        << "trailing CR leaked into the mtllib path (MTL failed to resolve)";
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

TEST_F(OBJTest, MTLPresent_NoMapKd_PreservesEmptyChartSlot)
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

    // The used material (chart 0) has no map_Kd, but its slot is preserved as
    // an empty path so texture_paths stays indexed by chart index.
    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_TRUE(dst_textures[0].empty());
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

// A WithNormal mesh whose vertices carry no normals must not declare
// nx/ny/nz in the header nor write fabricated zero normals. PLY's
// fixed-property element makes this an all-or-nothing header decision.
TEST_F(PLYTest, NormalCapableButNoneSet_DeclaresNoNormals)
{
    NormalMesh src;
    (void)src.insert_vertex(0.f, 0.f, 0.f);
    (void)src.insert_vertex(1.f, 0.f, 0.f);
    (void)src.insert_vertex(0.f, 1.f, 0.f);
    (void)src.insert_face(0u, 1u, 2u);  // no normals set

    const auto path = ply("no_normals");
    write_ply(path, src);

    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        EXPECT_EQ(line.find("property float nx"), std::string::npos)
            << "Unexpected nx property for normal-less mesh: " << line;
        EXPECT_EQ(line.find("property float ny"), std::string::npos);
        EXPECT_EQ(line.find("property float nz"), std::string::npos);
    }

    NormalMesh dst;
    read_ply(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    EXPECT_FALSE(dst.vertex(0).normal.has_value());
    EXPECT_FALSE(dst.vertex(1).normal.has_value());
    EXPECT_FALSE(dst.vertex(2).normal.has_value());
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

// A WithColor mesh whose vertices carry no colors must not declare
// red/green/blue in the header nor write fabricated black. Like normals,
// PLY's fixed-property element makes this an all-or-nothing header decision.
TEST_F(PLYTest, ColorCapableButNoneSet_DeclaresNoColors)
{
    ColorMesh src;
    (void)src.insert_vertex(0.f, 0.f, 0.f);
    (void)src.insert_vertex(1.f, 0.f, 0.f);
    (void)src.insert_vertex(0.f, 1.f, 0.f);
    (void)src.insert_face(0u, 1u, 2u);  // no colors set

    const auto path = ply("no_colors");
    write_ply(path, src);

    std::ifstream f(path);
    std::string line;
    while (std::getline(f, line)) {
        EXPECT_EQ(line.find("property uchar red"), std::string::npos)
            << "Unexpected red property for color-less mesh: " << line;
        EXPECT_EQ(line.find("property uchar green"), std::string::npos);
        EXPECT_EQ(line.find("property uchar blue"), std::string::npos);
    }

    ColorMesh dst;
    read_ply(path, dst);
    ASSERT_EQ(dst.num_vertices(), 3u);
    EXPECT_FALSE(dst.vertex(0).color.has_value());
    EXPECT_FALSE(dst.vertex(1).color.has_value());
    EXPECT_FALSE(dst.vertex(2).color.has_value());
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

TEST_F(PLYTest, SizedTypeAliases_ASCII_Read)
{
    // Many third-party writers spell property types with the sized aliases
    // (float32, uint8, ...) rather than the names in the original PLY spec.
    // These files are never produced by write_ply, so only a hand-crafted
    // header exercises the alias path.
    const auto path = ply("sized_ascii");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 3\n"
          << "property float32 x\n"
          << "property float32 y\n"
          << "property float32 z\n"
          << "property float64 nx\n"
          << "property float64 ny\n"
          << "property float64 nz\n"
          << "property uint8 red\n"
          << "property uint8 green\n"
          << "property uint8 blue\n"
          << "element face 1\n"
          << "property list uint8 uint32 vertex_indices\n"
          << "end_header\n"
          << "0 0 0 0 0 1 255 0 0\n"
          << "1 0 0 0 0 1 0 255 0\n"
          << "0 1 0 0 0 1 0 0 255\n"
          << "3 0 1 2\n";
    }

    NCMesh dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_EQ(dst.num_faces(), 1u);
    EXPECT_NEAR(dst.vertex(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[1], 1.f, 1e-5f);
    ASSERT_TRUE(dst.vertex(0).normal.has_value());
    EXPECT_NEAR((*dst.vertex(0).normal)[2], 1.f, 1e-5f);
    ASSERT_TRUE(dst.vertex(0).color.has_value());
    EXPECT_EQ(dst.vertex(0).color.value<Color::U8C3>()[0], 255u);
    EXPECT_EQ(dst.vertex(1).color.value<Color::U8C3>()[1], 255u);
    EXPECT_EQ(dst.vertex(2).color.value<Color::U8C3>()[2], 255u);
    EXPECT_EQ(dst.face(0), (NCMesh::Face{0, 1, 2}));
}

TEST_F(PLYTest, SizedTypeAliases_BinaryLittleEndian_Read)
{
    // Header shape emitted by OpenMVS. The alias names must also resolve to
    // the correct byte widths, or the binary reader desynchronizes.
    const auto path = ply("sized_binary");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element vertex 3\n"
          << "property float32 x\n"
          << "property float32 y\n"
          << "property float32 z\n"
          << "element face 1\n"
          << "property list uint8 uint32 vertex_indices\n"
          << "end_header\n";
        const float verts[9] = {
            0.f, 0.f, 0.f,
            1.f, 0.f, 0.f,
            0.f, 1.f, 0.f};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
        const uint8_t  cnt = 3;
        const uint32_t idx[3] = {0, 1, 2};
        f.write(reinterpret_cast<const char*>(&cnt), 1);
        f.write(reinterpret_cast<const char*>(idx), sizeof(idx));
    }

    Mesh3f dst;
    read_ply(path, dst);

    ASSERT_EQ(dst.num_vertices(), 3u);
    ASSERT_EQ(dst.num_faces(), 1u);
    EXPECT_NEAR(dst.vertex(1)[0], 1.f, 1e-5f);
    EXPECT_NEAR(dst.vertex(2)[1], 1.f, 1e-5f);
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

// A "comment TextureFile" path may contain spaces; the reader must capture the
// whole line remainder, not just the first token.
TEST_F(PLYTest, ReadCommentTextureFile_FilenameWithSpaces)
{
    const auto path = ply("spaced_texture");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "comment TextureFile my atlas file.png\n"
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

    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_EQ(dst_textures[0], fs::path("my atlas file.png"));
}

// CRLF line endings must not leak a trailing '\r' into a captured texture path.
TEST_F(PLYTest, ReadCommentTextureFile_CRLFLineEndings)
{
    const auto path = ply("crlf_texture");
    {
        // Write with explicit CRLF terminators (binary mode to preserve them).
        std::ofstream f(path, std::ios::binary);
        f << "ply\r\n"
          << "format ascii 1.0\r\n"
          << "comment TextureFile atlas.png\r\n"
          << "element vertex 3\r\n"
          << "property float x\r\n"
          << "property float y\r\n"
          << "property float z\r\n"
          << "element face 1\r\n"
          << "property list uchar int vertex_indices\r\n"
          << "end_header\r\n"
          << "0 0 0\r\n"
          << "1 0 0\r\n"
          << "0 1 0\r\n"
          << "3 0 1 2\r\n";
    }

    Mesh3f dst_mesh;
    UVMap2f dst_uv;
    std::vector<fs::path> dst_textures;
    read_ply(path, dst_mesh, dst_uv, dst_textures);

    ASSERT_EQ(dst_textures.size(), 1u);
    EXPECT_EQ(dst_textures[0], fs::path("atlas.png"))
        << "trailing CR leaked into the texture path";
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
//------------------------------------------------------------------------------
// Reader robustness: a malformed or unusual file must produce an error, never
// a silently wrong mesh. These are the worst failure mode for a reader — the
// caller gets geometry back and has no way to know it is garbage.
//------------------------------------------------------------------------------

TEST_F(PLYTest, UnknownElementSignedListCount_Throws)
{
    // PLY permits a signed list-count type. A count byte of 0xFF read as
    // `char` is -1, which as an unsigned byte-count is astronomically large;
    // multiplied by the element width it wraps back around to a small negative
    // number, and a seek of that size skips nothing at all. The reader then
    // parses this element's payload as vertex data.
    //
    // There are enough bytes left for that to succeed, so without a bound the
    // read returns two vertices of junk and reports no error.
    const auto path = ply("unknown_signed_count");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element blob 1\n"
          << "property list char double junk\n"
          << "element vertex 2\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        const uint8_t count = 0xFF;  // -1 as char
        f.write(reinterpret_cast<const char*>(&count), 1);
        const double junk[3] = {-9.5, -8.5, -7.5};
        f.write(reinterpret_cast<const char*>(junk), sizeof(junk));
        const float verts[6] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
    }

    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

TEST_F(PLYTest, UnknownElementOversizeListCount_Throws)
{
    // The same guard from the other side: a count that is genuinely huge
    // rather than negative. This already failed before the bound existed, but
    // only by running off the end of the file, so it is pinned here to make
    // sure it now fails on the count itself.
    const auto path = ply("unknown_huge_count");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element blob 1\n"
          << "property list uint double junk\n"
          << "element vertex 1\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        const uint32_t count = 0xFFFFFFFFu;
        f.write(reinterpret_cast<const char*>(&count), 4);
        const float verts[3] = {1.f, 2.f, 3.f};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
    }

    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

TEST_F(PLYTest, UnknownElementValidListCount_SkipsCorrectly)
{
    // The bound must not break the case it guards. A well-formed unknown
    // element is skipped and the vertices that follow read correctly.
    const auto path = ply("unknown_valid_count");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element blob 1\n"
          << "property list char double junk\n"
          << "element vertex 2\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        const uint8_t count = 3;
        f.write(reinterpret_cast<const char*>(&count), 1);
        const double junk[3] = {-9.5, -8.5, -7.5};
        f.write(reinterpret_cast<const char*>(junk), sizeof(junk));
        const float verts[6] = {1.f, 2.f, 3.f, 4.f, 5.f, 6.f};
        f.write(reinterpret_cast<const char*>(verts), sizeof(verts));
    }

    Mesh3f dst;
    read_ply(path, dst);
    ASSERT_EQ(dst.num_vertices(), 2u);
    EXPECT_NEAR(dst.vertex(0)[0], 1.f, 1e-6f);
    EXPECT_NEAR(dst.vertex(0)[1], 2.f, 1e-6f);
    EXPECT_NEAR(dst.vertex(0)[2], 3.f, 1e-6f);
    EXPECT_NEAR(dst.vertex(1)[0], 4.f, 1e-6f);
    EXPECT_NEAR(dst.vertex(1)[1], 5.f, 1e-6f);
    EXPECT_NEAR(dst.vertex(1)[2], 6.f, 1e-6f);
}

TEST_F(PLYTest, VertexElementWithListProperty_Throws)
{
    // The binary vertex reader sizes each record by summing its properties'
    // element widths, which is only correct when every property is a single
    // scalar. A list property occupies a count plus N values, so the record
    // size comes out short, every read is misaligned, and vertices after the
    // first are garbage — with no error.
    //
    // Neither the binary nor the ASCII vertex path interprets a list property,
    // so the file is refused rather than half-read.
    const auto path = ply("vertex_list_prop");
    {
        std::ofstream f(path, std::ios::binary);
        f << "ply\n"
          << "format binary_little_endian 1.0\n"
          << "element vertex 2\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "property list uchar int extra\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n";
        for (int vi = 0; vi < 2; ++vi) {
            const float v[3] = {
                static_cast<float>(3 * vi + 1),
                static_cast<float>(3 * vi + 2),
                static_cast<float>(3 * vi + 3)};
            f.write(reinterpret_cast<const char*>(v), sizeof(v));
            const uint8_t n = 1;
            const int32_t val = 7;
            f.write(reinterpret_cast<const char*>(&n), 1);
            f.write(reinterpret_cast<const char*>(&val), 4);
        }
    }

    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

TEST_F(PLYTest, VertexElementWithListProperty_ASCII_Throws)
{
    // Same declaration, ASCII. The ASCII vertex loop indexes tokens by
    // property position, which a list property also breaks, so it is refused
    // for the same reason.
    const auto path = ply("vertex_list_prop_ascii");
    {
        std::ofstream f(path);
        f << "ply\n"
          << "format ascii 1.0\n"
          << "element vertex 2\n"
          << "property float x\n"
          << "property float y\n"
          << "property float z\n"
          << "property list uchar int extra\n"
          << "element face 0\n"
          << "property list uchar int vertex_indices\n"
          << "end_header\n"
          << "1 2 3 1 7\n"
          << "4 5 6 1 7\n";
    }

    Mesh3f dst;
    EXPECT_THROW(read_ply(path, dst), std::runtime_error);
}

// A write failure that happens only in the final flush.
//
// write_ply checks the stream while the tail of the data may still be
// buffered; the flush happens when the ofstream is destroyed, and a failure
// there is swallowed, so write_ply returns normally on an incomplete file.
//
// Isolating that needs the failure to occur at close and nowhere earlier. With
// RLIMIT_FSIZE at 0 every write to the file fails, and with a mesh smaller than
// the stream buffer no write is attempted until close — so the stream is still
// good when write_ply's check runs, and only the flush fails.
//
// POSIX-only: there is no portable way to provoke this.
#if defined(__unix__) || defined(__APPLE__)
TEST_F(PLYTest, WriteFailureInFinalFlush_Throws)
{
    // Exceeding RLIMIT_FSIZE raises SIGXFSZ, which by default kills the
    // process; ignore it so the write reports EFBIG instead.
    struct Guard {
        rlimit saved{};
        void (*prev_sigxfsz)(int){nullptr};
        bool active{false};
        Guard()
        {
            if (::getrlimit(RLIMIT_FSIZE, &saved) != 0) {
                return;
            }
            prev_sigxfsz = std::signal(SIGXFSZ, SIG_IGN);
            rlimit zero{0, saved.rlim_max};
            active = ::setrlimit(RLIMIT_FSIZE, &zero) == 0;
        }
        ~Guard()
        {
            if (active) {
                (void)::setrlimit(RLIMIT_FSIZE, &saved);
            }
            if (prev_sigxfsz != nullptr) {
                (void)std::signal(SIGXFSZ, prev_sigxfsz);
            }
        }
    } guard;
    if (!guard.active) {
        GTEST_SKIP() << "cannot lower RLIMIT_FSIZE in this environment";
    }

    // A triangle is a few hundred bytes — comfortably inside the stream
    // buffer, so nothing reaches the filesystem before close.
    const auto src  = make_triangle();
    const auto path = ply("flush_fails");
    EXPECT_THROW(write_ply(path, src), std::runtime_error);
}
#endif

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

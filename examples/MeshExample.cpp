#include <iostream>

#include "educelab/core/types/Mesh.hpp"
#include "educelab/core/types/UVMap.hpp"

using namespace educelab;

auto main() -> int
{
    //--------------------------------------------------------------------------
    // Basic mesh: positions only
    //--------------------------------------------------------------------------
    // Default Mesh<float, 3> — no extra vertex traits
    Mesh<float, 3> mesh;

    // Insert vertices (returns index)
    auto v0 = mesh.insert_vertex(0.F, 0.F, 0.F);
    auto v1 = mesh.insert_vertex(1.F, 0.F, 0.F);
    auto v2 = mesh.insert_vertex(1.F, 1.F, 0.F);
    auto v3 = mesh.insert_vertex(0.F, 1.F, 0.F);

    // Insert a quad face (N-gon faces are supported)
    auto f0 = mesh.insert_face(v0, v1, v2, v3);

    std::cout << "Vertex 0: " << mesh.vertex(v0) << "\n";  // [0, 0, 0]
    std::cout << "Vertex 2: " << mesh.vertex(v2) << "\n";  // [1, 1, 0]

    // Adjacency: which faces contain vertex 1?
    for (auto fi : mesh.vertex_faces(v1)) {
        std::cout << "Vertex 1 in face " << fi << "\n";  // face 0
    }

    // Face normal (computed on demand, cached)
    std::cout << "Face 0 normal: " << mesh.face_normal(f0) << "\n";  // [0, 0, 1]

    //--------------------------------------------------------------------------
    // Mesh with normals and colors (composable traits)
    //--------------------------------------------------------------------------
    struct MyTraits : traits::WithNormal<float, 3>, traits::WithColor {};
    using RichMesh = Mesh<float, 3, MyTraits>;

    RichMesh rich;
    auto rv0 = rich.insert_vertex(0.F, 0.F, 0.F);
    auto rv1 = rich.insert_vertex(1.F, 0.F, 0.F);
    auto rv2 = rich.insert_vertex(0.5F, 1.F, 0.F);
    rich.insert_face(rv0, rv1, rv2);

    rich.vertex(rv0).normal = Vec3f{0.F, 0.F, 1.F};
    rich.vertex(rv0).color  = Color::U8C3{255, 0, 0};

    auto& vert = rich.vertex(rv0);
    auto rgb = vert.color.value<Color::U8C3>();
    std::cout << "Normal: " << vert.normal.value() << "\n";  // [0, 0, 1]
    std::cout << "Color:  [" << int(rgb[0]) << ", " << int(rgb[1]) << ", "
              << int(rgb[2]) << "]\n";  // [255, 0, 0]

    //--------------------------------------------------------------------------
    // UVMap: per-wedge UV coordinates
    //--------------------------------------------------------------------------
    // UVMap<float, 2> — default 2D UV, no chart index
    UVMap<float, 2> uvmap;

    // Insert UV coordinates into the pool (returns pool index)
    auto uv0 = uvmap.insert(0.0F, 0.0F);
    auto uv1 = uvmap.insert(1.0F, 0.0F);
    auto uv2 = uvmap.insert(1.0F, 1.0F);
    auto uv3 = uvmap.insert(0.0F, 1.0F);

    std::cout << "UV pool size: " << uvmap.size() << "\n";  // 4

    // Assign pool entries to per-face corners (face index, corner index, UV index)
    uvmap.map(f0, 0, uv0);
    uvmap.map(f0, 1, uv1);
    uvmap.map(f0, 2, uv2);
    uvmap.map(f0, 3, uv3);

    // Look up the coordinate for face 0, corner 2
    const auto& coord = uvmap.get_coordinate(f0, 2);
    std::cout << "UV at (face=0, corner=2): [" << coord[0] << ", " << coord[1]
              << "]\n";  // [1, 1]

    // Seam support: the same vertex can have different UVs in adjacent faces
    auto v4 = mesh.insert_vertex(0.5F, 0.5F, 0.F);
    auto f1 = mesh.insert_face(v0, v4, v3);
    auto uv_seam_a = uvmap.insert(0.5F, 0.5F);
    auto uv_seam_b = uvmap.insert(0.5F, 0.6F);  // same vertex, different UV
    uvmap.map(f0, 0, uv_seam_a);
    uvmap.map(f1, 0, uv_seam_b);
    std::cout << "Seam UV on face 0: " << uvmap.get_coordinate(f0, 0)[0] << "\n";  // 0.5
    std::cout << "Seam UV on face 1: " << uvmap.get_coordinate(f1, 0)[1] << "\n";  // 0.6

    //--------------------------------------------------------------------------
    // UVMap with chart index (multi-texture atlas)
    //--------------------------------------------------------------------------
    using ChartedUVMap = UVMap<float, 2, traits::WithChart>;
    ChartedUVMap charted;

    // Coordinate on chart 0
    ChartedUVMap::Coordinate c0(0.25F, 0.25F);
    c0.chart = 0;
    auto ci0 = charted.insert(c0);

    // Coordinate on chart 1
    ChartedUVMap::Coordinate c1(0.75F, 0.75F);
    c1.chart = 1;
    auto ci1 = charted.insert(c1);

    charted.map(0, 0, ci0);
    charted.map(0, 1, ci1);

    std::cout << "UV for corner 0: [" << charted.get_coordinate(0, 0)[0]
              << ", " << charted.get_coordinate(0, 0)[1]
              << "] on chart " << charted.get_coordinate(0, 0).chart << "\n";  // chart 0
    std::cout << "UV for corner 1: [" << charted.get_coordinate(0, 1)[0]
              << ", " << charted.get_coordinate(0, 1)[1]
              << "] on chart " << charted.get_coordinate(0, 1).chart << "\n";  // chart 1
}

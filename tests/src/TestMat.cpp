#include <gtest/gtest.h>

#include "educelab/core/types/Mat.hpp"
#include "educelab/core/types/Vec.hpp"
#include "educelab/core/utils/Iteration.hpp"

using namespace educelab;

using Mat3f = Mat<3, 3>;

TEST(Mat, DefaultConstructor)
{
    Mat3f m;
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_FLOAT_EQ(m(y, x), 0.F);
    }
}

TEST(Mat, FillConstructor)
{
    Mat3f m{0, 1, 2, 3, 4, 5, 6, 7, 8};
    for (auto i : range(9)) {
        EXPECT_FLOAT_EQ(m.data()[i], float(i));
    }
}

TEST(Mat, At)
{
    Mat3f m;
    // In bounds access (non-const)
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_NO_THROW(m.at(y, x) = float(y * m.cols + x));
    }

    // In bounds access (const)
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_FLOAT_EQ(m.at(y, x), float(y * m.cols + x));
    }

    // Out of bounds access
    EXPECT_THROW(m.at(3, 3), std::out_of_range);
}

TEST(Mat, AccessOperator)
{
    Mat3f m;
    // In bounds access (non-const)
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_NO_FATAL_FAILURE(m(y, x) = float(y * m.cols + x));
    }

    // In bounds access (const)
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_FLOAT_EQ(m(y, x), float(y * m.cols + x));
    }
}

TEST(Mat, Transpose)
{
    // Build Mat
    Mat3f m{0, 1, 2, 3, 4, 5, 6, 7, 8};
    m = m.t();

    // Iterate, but transpose indices
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_FLOAT_EQ(m.at(x, y), float(y * m.cols + x));
    }
}

TEST(Mat, Eye)
{
    auto m = Mat3f::Eye();
    for (auto [y, x] : range2D(3, 3)) {
        EXPECT_FLOAT_EQ(m.at(y, x), (y == x) ? 1.F : 0.F);
    }
}

TEST(Mat, MatrixMatrixMultiplication)
{
    // Square multiplication
    Mat<2, 2> m0{1, 2, 3, 4};
    Mat<2, 2> m1{5, 6, 7, 8};
    auto result = m0 * m1;

    // Check result
    Mat<2, 2> expected{19, 22, 43, 50};
    for (auto [y, x] : range2D(2, 2)) {
        EXPECT_FLOAT_EQ(result.at(y, x), expected.at(y, x));
    }

    // Non-square multiplication
    Mat<2, 3> m2{1, 2, 3, 4, 5, 6};
    Mat<3, 2> m3{7, 8, 9, 10, 11, 12};
    result = m2 * m3;

    // Check result
    expected = Mat<2, 2>{58, 64, 139, 154};
    for (auto [y, x] : range2D(2, 2)) {
        EXPECT_FLOAT_EQ(result.at(y, x), expected.at(y, x));
    }
}

TEST(Mat, MatrixVectorMultiplication)
{
    using Vec4f = Vec<float, 4>;

    // Input point
    Vec4f x{0, 0, 0, 1};

    // Translation matrix
    auto M = Mat<4, 4>::Eye();
    M(0, 3) = 1.F;
    M(1, 3) = 2.F;
    M(2, 3) = 3.F;
    auto result = M * x;
    EXPECT_EQ(result, Vec4f(1, 2, 3, 1));
}

TEST(Mat, Determinant2x2)
{
    Mat<2, 2> m{1, 2, 3, 4};
    EXPECT_FLOAT_EQ(determinant(m), -2.F);
}

TEST(Mat, Determinant3x3)
{
    Mat3f m{1, 2, 3, 4, 5, 6, 7, 8, 9};
    EXPECT_FLOAT_EQ(determinant(m), 0.F);
}
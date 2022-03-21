#include <gtest/gtest.h>

#include "educelab/core/types/Mat.hpp"
#include "educelab/core/types/Vec.hpp"
#include "educelab/core/utils/LinearAlgebra.hpp"

using namespace educelab;
using namespace educelab::linalg;

TEST(LinearAlgebra, SolveCramer)
{
    Mat<3, 3> A{2, 1, 1, 1, -1, -1, 1, 2, 1};
    Vec3f b{3, 0, 0};
    Vec3f x;
    EXPECT_NO_THROW(x = solve_cramer(A, b));
    EXPECT_EQ(x, Vec3f(1, -2, 3));
}

TEST(LinearAlgebra, SolveCramerError)
{
    Mat<3, 3> A{1, 1, 1, 1, 1, 2, 1, 1, 3};
    Vec3f b{1, 3, -1};
    EXPECT_THROW(solve_cramer(A, b), std::runtime_error);
}

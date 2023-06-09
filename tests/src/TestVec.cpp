#include <gtest/gtest.h>

#include "educelab/core.hpp"

using namespace educelab;

TEST(Vec, OperatorPlus)
{
    Vec3f a{1, 1, 1};
    Vec3f b{1, 1, 1};
    EXPECT_EQ(a + b, Vec3f(2, 2, 2));
    EXPECT_EQ(a, Vec3f(1, 1, 1));
    EXPECT_EQ(b, Vec3f(1, 1, 1));
    EXPECT_EQ(a += b, Vec3f(2, 2, 2));
    EXPECT_EQ(a, Vec3f(2, 2, 2));

    // Initializer list
    a += {1, 1, 1};
    EXPECT_EQ(a, Vec3f(3, 3, 3));
}

TEST(Vec, OperatorMinus)
{
    Vec3f a{1, 1, 1};
    Vec3f b{1, 1, 1};
    EXPECT_EQ(a - b, Vec3f(0, 0, 0));
    EXPECT_EQ(a, Vec3f(1, 1, 1));
    EXPECT_EQ(b, Vec3f(1, 1, 1));
    EXPECT_EQ(a -= b, Vec3f(0, 0, 0));
    EXPECT_EQ(a, Vec3f(0, 0, 0));

    // Initializer list
    a -= {1, 1, 1};
    EXPECT_EQ(a, Vec3f(-1, -1, -1));
}

TEST(Vec, OperatorMultiply)
{
    Vec3f a{1, 1, 1};
    EXPECT_EQ(a * 2, Vec3f(2, 2, 2));
    EXPECT_EQ(2 * a, Vec3f(2, 2, 2));
    EXPECT_EQ(a, Vec3f(1, 1, 1));
    EXPECT_EQ(a *= 2, Vec3f(2, 2, 2));
    EXPECT_EQ(a, Vec3f(2, 2, 2));
    EXPECT_EQ(-a, Vec3f(-2, -2, -2));
    EXPECT_EQ(a, Vec3f(2, 2, 2));
}

TEST(Vec, OperatorDivide)
{
    Vec3f a{2, 2, 2};
    EXPECT_EQ(a / 2, Vec3f(1, 1, 1));
    EXPECT_EQ(2 / a, Vec3f(1, 1, 1));
    EXPECT_EQ(a, Vec3f(2, 2, 2));
    EXPECT_EQ(a /= 2, Vec3f(1, 1, 1));
    EXPECT_EQ(a, Vec3f(1, 1, 1));
}

TEST(Vec, DotProduct)
{
    EXPECT_EQ(Vec3f(1, 0, 0).dot(Vec3f(0, 1, 0)), 0);
    EXPECT_EQ(Vec3f(1, 0, 0).dot(Vec3f(0, 0, 1)), 0);
    EXPECT_EQ(Vec3f(0, 1, 0).dot(Vec3f(0, 0, 1)), 0);

    EXPECT_EQ(Vec3f(1, 0, 0).dot(Vec3f(1, 0, 0)), 1);
    EXPECT_EQ(Vec3f(0, 1, 0).dot(Vec3f(0, 1, 0)), 1);
    EXPECT_EQ(Vec3f(0, 0, 1).dot(Vec3f(0, 0, 1)), 1);

    // Initializer list
    EXPECT_EQ(Vec3f(1, 0, 0).dot({1, 0, 0}), 1);
    EXPECT_EQ(Vec3f(0, 1, 0).dot({0, 1, 0}), 1);
    EXPECT_EQ(Vec3f(0, 0, 1).dot({0, 0, 1}), 1);
}

TEST(Vec, CrossProduct)
{
    EXPECT_EQ(Vec3f(1, 0, 0).cross(Vec3f(1, 0, 0)), Vec3f(0, 0, 0));
    EXPECT_EQ(Vec3f(1, 0, 0).cross(Vec3f(0, 1, 0)), Vec3f(0, 0, 1));
    EXPECT_EQ(Vec3f(1, 0, 0).cross(Vec3f(0, 0, 1)), Vec3f(0, -1, 0));

    // Initializer list
    EXPECT_EQ(Vec3f(1, 0, 0).cross({1, 0, 0}), Vec3f(0, 0, 0));
    EXPECT_EQ(Vec3f(1, 0, 0).cross({0, 1, 0}), Vec3f(0, 0, 1));
    EXPECT_EQ(Vec3f(1, 0, 0).cross({0, 0, 1}), Vec3f(0, -1, 0));
}

TEST(Vec, Magnitude)
{
    EXPECT_EQ(Vec3f(1, 0, 0).magnitude(), 1.f);
    EXPECT_EQ(Vec3f(0, 2, 0).magnitude(), 2.f);
    EXPECT_EQ(Vec3f(0, 0, 3).magnitude(), 3.f);
}

TEST(Vec, Magnitude2)
{
    EXPECT_EQ(Vec3f(1, 0, 0).magnitude2(), 1.f);
    EXPECT_EQ(Vec3f(0, 2, 0).magnitude2(), 4.f);
    EXPECT_EQ(Vec3f(0, 0, 3).magnitude2(), 9.f);
}

TEST(Vec, UnitVector)
{
    Vec3f a{2, 0, 0};
    EXPECT_EQ(a.unit(), Vec3f(1, 0, 0));
    EXPECT_EQ(a, Vec3f(2, 0, 0));
}

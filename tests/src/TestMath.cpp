#include <gtest/gtest.h>

#include <limits>

#include "educelab/core.hpp"

using namespace educelab;

TEST(Math, Constants)
{
    EXPECT_FLOAT_EQ(PI<float>, 3.141592653589793238462643383279502884198716939937510582097164L);
    EXPECT_DOUBLE_EQ(PI<double>, 3.141592653589793238462643383279502884198716939937510582097164L);

    EXPECT_FLOAT_EQ(INF<float>, std::numeric_limits<float>::infinity());
    EXPECT_DOUBLE_EQ(INF<double>, std::numeric_limits<double>::infinity());
}

TEST(Math, DotProduct)
{
    EXPECT_EQ(dot(Vec3f{1, 0, 0}, Vec3f{0, 1, 0}), 0);
    EXPECT_EQ(dot(Vec3f{1, 0, 0}, Vec3f{0, 0, 1}), 0);
    EXPECT_EQ(dot(Vec3f{0, 1, 0}, Vec3f{0, 0, 1}), 0);

    EXPECT_EQ(dot(Vec3f{1, 0, 0}, Vec3f{1, 0, 0}), 1);
    EXPECT_EQ(dot(Vec3f{0, 1, 0}, Vec3f{0, 1, 0}), 1);
    EXPECT_EQ(dot(Vec3f{0, 0, 1}, Vec3f{0, 0, 1}), 1);
}

TEST(Math, CrossProduct)
{
    EXPECT_EQ(cross(Vec3f{1, 0, 0}, Vec3f{1, 0, 0}), Vec3f(0, 0, 0));
    EXPECT_EQ(cross(Vec3f{1, 0, 0}, Vec3f{0, 1, 0}), Vec3f(0, 0, 1));
    EXPECT_EQ(cross(Vec3f{1, 0, 0}, Vec3f{0, 0, 1}), Vec3f(0, -1, 0));
}

TEST(Math, Norm)
{
    Vec3f vec{1, 0, 0};
    EXPECT_EQ(norm(vec, Norm::L1), 1.f);
    EXPECT_EQ(norm(vec, Norm::L2), 1.f);
    EXPECT_EQ(norm(vec, Norm::LInf), 1.f);
}

TEST(Math, Normalize)
{
    EXPECT_EQ(normalize(Vec3f(1, 0, 0)), Vec3f(1, 0, 0));
    EXPECT_EQ(normalize(Vec3f(0, 2, 0)), Vec3f(0, 1, 0));
    EXPECT_EQ(normalize(Vec3f(0, 0, 3)), Vec3f(0, 0, 1));
}

TEST(Math, AngleConversion)
{
    EXPECT_EQ(to_radians(180), PI<float>);
    EXPECT_EQ(to_degrees(PI<float>), 180.f);
}

TEST(Math, InteriorAngle)
{
    using Vec2f = Vec<float, 2>;
    EXPECT_EQ(interior_angle(Vec2f{1, 0}, Vec2f{0, 1}), to_radians(90));
    EXPECT_EQ(interior_angle(Vec3f{1, 0, 0}, Vec3f{0, 1, 0}), to_radians(90));
}
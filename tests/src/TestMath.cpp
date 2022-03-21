#include <gtest/gtest.h>

#include <limits>

#include "educelab/core.hpp"

using namespace educelab;

TEST(Math, Constants)
{
    EXPECT_FLOAT_EQ(
        PI<float>,
        3.141592653589793238462643383279502884198716939937510582097164L);
    EXPECT_DOUBLE_EQ(
        PI<double>,
        3.141592653589793238462643383279502884198716939937510582097164L);

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

    EXPECT_EQ(cross(Vec3f{1, 0, 0}, {1, 0, 0}), Vec3f(0, 0, 0));
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

TEST(Math, RandomFloatDefault)
{
    for ([[maybe_unused]] const auto r : range(1000)) {
        auto val = random<float>();
        EXPECT_GE(val, 0.F);
        EXPECT_LT(val, 1.F);
    }
}

TEST(Math, RandomFloatCustom)
{
    for ([[maybe_unused]] const auto r : range(1000)) {
        auto val = random(0.F, 10.F);
        EXPECT_GE(val, 0.F);
        EXPECT_LT(val, 10.F);
    }
}

TEST(Math, RandomDoubleDefault)
{
    for ([[maybe_unused]] const auto r : range(1000)) {
        auto val = random<double>();
        EXPECT_GE(val, 0.);
        EXPECT_LT(val, 1.);
    }
}

TEST(Math, RandomDoubleCustom)
{
    for ([[maybe_unused]] const auto r : range(1000)) {
        auto val = random(0., 10.);
        EXPECT_GE(val, 0.);
        EXPECT_LT(val, 10.);
    }
}

TEST(Math, AlmostZero)
{
    EXPECT_TRUE(almost_zero(1e-8F));
    EXPECT_FALSE(almost_zero(1e-7F));
}

TEST(Math, SolveQuadraticReal)
{
    if (auto res = solve_quadratic(5.F, 6.F, 1.F)) {
        EXPECT_FLOAT_EQ(res.t0, -1.F);
        EXPECT_FLOAT_EQ(res.t1, -0.2F);
    }
}

TEST(Math, SolveQuadraticImaginary)
{
    EXPECT_FALSE(solve_quadratic(5.F, 2.F, 1.F));
}

TEST(Math, SolveQuadraticLinear)
{
    EXPECT_THROW(solve_quadratic(0.F, 2.F, 1.F), std::invalid_argument);
}

TEST(Math, SchurProduct)
{
    Vec3f a{1, 2, 3};
    Vec3f b{4, 5, 6};
    auto result = schur_product(a, b);
    EXPECT_EQ(result, Vec3f(4, 10, 18));
}
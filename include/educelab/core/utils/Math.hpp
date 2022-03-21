#pragma once

/** @file */

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <random>

namespace educelab
{

/** @brief Pi, templated for floating-point type */
template <
    class T,
    std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
constexpr T PI =
    T(3.141592653589793238462643383279502884198716939937510582097164L);

/** @brief Inf, templated for floating-point type */
template <
    class T,
    std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
constexpr T INF = std::numeric_limits<T>::infinity();

/** @brief Vector dot product (inner product) */
template <typename T1, typename T2>
auto dot(const T1& a, const T2& b)
{
    if (std::size(a) != std::size(b)) {
        throw std::invalid_argument("Inputs have mismatched dimensions");
    }
    using Ret = decltype(*std::begin(a));
    return std::inner_product(
        std::begin(a), std::end(a), std::begin(b), Ret(0));
}

/** @brief Vector cross product */
template <typename T1, typename T2>
auto cross(const T1& a, const T2& b) -> T1
{
    if (std::size(a) != 3 or std::size(b) != 3) {
        throw std::invalid_argument("Inputs have mismatched dimensions");
    }
    T1 c;
    c[0] = a[1] * b[2] - a[2] * b[1];
    c[1] = a[2] * b[0] - a[0] * b[2];
    c[2] = a[0] * b[1] - a[1] * b[0];
    return c;
}

/** @brief Element-wise vector product */
template <typename T1, typename T2>
auto schur_product(const T1& a, const T2& b) -> T1
{
    T1 res;
    std::transform(
        std::begin(a), std::end(a), std::begin(b), std::begin(res),
        std::multiplies<typename T1::value_type>());
    return res;
}

/** @brief Vector cross product for initializer list */
template <
    typename T1,
    typename T2,
    std::enable_if_t<std::is_arithmetic<T2>::value, bool> = true>
auto cross(const T1& a, const std::initializer_list<T2>& b) -> T1
{
    struct InitList : public std::initializer_list<T2> {
        using Base = std::initializer_list<T2>;
        InitList(const Base& b) : Base{b} {}
        auto operator[](std::size_t idx) const -> const T2&
        {
            return *(Base::begin() + idx);
        }
    };
    return cross(a, InitList{b});
}

/** @brief Norm type enumeration */
enum class Norm {
    L1,  /**< \f$L_1\f$ norm */
    L2,  /**< \f$L_2\f$ norm */
    LInf /**< \f$L_{Inf}\f$ norm */
};

/** @brief Compute vector norm */
template <class Vector>
auto norm(const Vector& v, Norm norm = Norm::L2)
{
    using Ret = decltype(*std::begin(v));
    switch (norm) {
        case Norm::L1: {
            return std::accumulate(
                std::begin(v), std::end(v), Ret(0),
                [](auto a, auto b) { return a + std::abs(b); });
        }
        case Norm::L2: {
            auto sum = std::accumulate(
                std::begin(v), std::end(v), Ret(0),
                [](auto a, auto b) { return a + (b * b); });
            return std::sqrt(sum);
        }
        case Norm::LInf: {
            return std::abs(*std::max_element(
                std::begin(v), std::end(v),
                [](auto a, auto b) { return std::abs(a) < std::abs(b); }));
        }
    }
    throw std::invalid_argument("Invalid norm option");
}

/** @brief Normalize a vector (i.e. compute a unit vector) */
template <class Vector>
auto normalize(Vector v)
{
    return v / norm(v, Norm::L2);
}

/** @brief Compute the interior angle between two vectors */
template <class Vector1, class Vector2>
auto interior_angle(const Vector1& a, const Vector2& b)
{
    return std::acos(dot(a, b) / (norm(a, Norm::L2) * norm(b, Norm::L2)));
}

/** @brief Convert degrees to radians */
template <
    typename T = float,
    typename T2,
    std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
constexpr auto to_radians(T2 deg) -> T
{
    return deg * PI<T> / T(180);
}

/** @brief Convert radians to degrees */
template <
    typename T = float,
    typename T2,
    std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
constexpr auto to_degrees(T2 rad) -> T
{
    return rad * T(180) / PI<T>;
}

/**
 * @brief Generate a uniformly random number in the range [min, max)
 *
 * @note For each type T, this function statically defines a single std::mt19937
 * generator and seeds it from std::random_device. This behavior may not be
 * desirable for all applications.
 */
template <typename T>
inline auto random(T min = 0, T max = 1) -> T
{
    std::uniform_real_distribution<T> distribution(min, max);
    static std::random_device source;
    static std::mt19937 generator(source());
    return distribution(generator);
}

/** @brief Check if the given value is almost zero using an absolute epsilon */
template <
    typename T,
    std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
inline auto almost_zero(T v, T eps = 1e-7) -> bool
{
    return std::abs(v) < eps;
}

/**
 * @brief Solve for the solutions of a quadratic equation
 *
 * @note t0 and t1 are sorted from lowest to highest value.
 */
template <
    typename T,
    std::enable_if_t<std::is_floating_point<T>::value, bool> = true>
inline auto solve_quadratic(T a, T b, T c)
{
    // Zero a means the equation is linear
    if (almost_zero(a)) {
        throw std::invalid_argument("First quadratic coefficient is zero");
    }

    /** Structure for storing solutions to a quadratic equation */
    struct quadratic_result {
        /** Whether the solution is real or imaginary */
        bool is_real{false};
        /** @copydoc is_real */
        explicit operator bool() const noexcept { return is_real; }
        /** First solution */
        T t0{INF<T>};
        /** Second solution */
        T t1{INF<T>};
    };

    // Calculate the discriminant
    auto dis = std::pow(b, 2) - 4 * a * c;
    if (dis < 0) {
        return quadratic_result{};
    }

    // Setup result
    quadratic_result res;
    res.is_real = true;

    // Zero discriminant means t0 and t1 are the same
    if (almost_zero(dis)) {
        res.t0 = res.t1 = -b / (T(2) * a);
    }

    // Numerically stable solution calculations
    // https://pbr-book.org/3ed-2018/Utilities/Mathematical_Routines#Quadratic
    else {
        auto disR = std::sqrt(dis);
        T q = (b < T(0)) ? T(-0.5) * (b - disR) : T(-0.5) * (b + disR);
        res.t0 = q / a;
        res.t1 = c / q;
    }

    // Sort solutions least to greatest
    if (res.t0 > res.t1) {
        std::swap(res.t0, res.t1);
    }

    return res;
}

}  // namespace educelab
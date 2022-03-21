#pragma once

/** @file */

#include <array>

#include "educelab/core/types/Vec.hpp"

namespace educelab
{

/**
 * @brief Dense 2D matrix class for linear algebra
 *
 * @tparam Rows Number of rows
 * @tparam Cols Number of columns
 * @tparam T Fundamental element type
 */
template <std::size_t Rows, std::size_t Cols, typename T = float>
class Mat
{
public:
    /** @brief Number of rows */
    static constexpr std::size_t rows{Rows};
    /** @brief Number of columns */
    static constexpr std::size_t cols{Cols};

    /** Default constructor */
    Mat() { vals_.fill(0); };

    /** @brief Constructor with fill values */
    template <typename... Args>
    explicit Mat(Args&&... args)
    {
        static_assert(
            sizeof...(args) == Rows * Cols, "Incorrect number of arguments");
        std::size_t i{0};
        ((vals_[i++] = args), ...);
    }

    /** @brief Matrix element access with bounds checking */
    auto at(std::size_t y, std::size_t x) -> T&
    {
        return vals_.at(Unravel(y, x));
    }

    /** @copydoc at(std::size_t, std::size_t) */
    auto at(std::size_t y, std::size_t x) const -> const T&
    {
        return vals_.at(Unravel(y, x));
    }

    /** @brief Matrix element access without bounds checking */
    auto operator()(std::size_t y, std::size_t x) -> T&
    {
        return vals_[Unravel(y, x)];
    }

    /** @copydoc operator()(std::size_t, std::size_t) */
    auto operator()(std::size_t y, std::size_t x) const -> const T&
    {
        return vals_[Unravel(y, x)];
    }

    /** @brief Return a transposed copy of the matrix */
    auto t() const -> Mat<Cols, Rows, T>
    {
        Mat<Cols, Rows, T> m;
        for (std::size_t y{0}; y < Rows; y++) {
            for (std::size_t x{0}; x < Cols; x++) {
                m(x, y) = vals_[Unravel(y, x)];
            }
        }
        return m;
    }

    /** @brief Construct a new identity matrix */
    static auto Eye() -> Mat
    {
        static_assert(Rows == Cols, "Matrix must be square");
        Mat m;
        for (std::size_t i{0}; i < Rows; i++) {
            m(i, i) = 1;
        }
        return m;
    }

    /** Access to underlying storage array */
    constexpr auto data() noexcept -> T* { return vals_.data(); }
    /** @copydoc data() */
    constexpr auto data() const noexcept -> const T* { return vals_.data(); }

private:
    /** Compute flat index */
    static inline auto Unravel(std::size_t y, std::size_t x)
    {
        return y * Cols + x;
    }
    /** Storage array */
    std::array<T, Rows * Cols> vals_{};
};

/** @brief Matrix-matrix multiplication operator */
template <typename T1, typename T2, std::size_t M, std::size_t N, std::size_t P>
auto operator*(const Mat<M, N, T1>& A, const Mat<N, P, T2>& B) -> Mat<M, P, T1>
{
    Mat<M, P, T1> res;
    for (std::size_t m{0}; m < M; m++) {
        for (std::size_t p{0}; p < P; p++) {
            for (std::size_t n{0}; n < N; n++) {
                res(m, p) += A(m, n) * B(n, p);
            }
        }
    }
    return res;
}

/** @brief Matrix-vector multiplication */
template <typename T1, typename T2, std::size_t M, std::size_t N>
auto operator*(const Mat<M, N, T1>& mat, const Vec<T2, N>& vec) -> Vec<T1, N>
{
    Vec<T1, N> res;
    for (std::size_t m{0}; m < M; m++) {
        for (std::size_t n{0}; n < N; n++) {
            res[m] += mat(m, n) * vec[n];
        }
    }
    return res;
}

/** @brief Calculate the 2x2 matrix determinant */
template <typename T>
auto determinant(const Mat<2, 2, T>& m) -> T
{
    return m.data()[0] * m.data()[3] - m.data()[1] * m.data()[2];
}

/** @brief Calculate the 3x3 matrix determinant */
template <typename T>
auto determinant(const Mat<3, 3, T>& m) -> T
{
    // a(ei − fh) − b(di − fg) + c(dh − eg)
    // 0(48 - 57) - 1(38 - 56) + 2(37 - 46)
    return m.data()[0] *
               (m.data()[4] * m.data()[8] - m.data()[5] * m.data()[7]) -
           m.data()[1] *
               (m.data()[3] * m.data()[8] - m.data()[5] * m.data()[6]) +
           m.data()[2] *
               (m.data()[3] * m.data()[7] - m.data()[4] * m.data()[6]);
}

/** Debug: Print a matrix to a std::ostream */
template <typename T, std::size_t Rows, std::size_t Cols>
auto operator<<(std::ostream& os, const Mat<Rows, Cols, T>& mat)
    -> std::ostream&
{
    os << "[";
    for (std::size_t y{0}; y < Rows; y++) {
        if (y != 0) {
            os << " ";
        }
        os << "[";
        for (std::size_t x{0}; x < Cols; x++) {
            if (x > 0) {
                os << ", ";
            }
            os << mat(y, x);
        }
        os << "]";
        if (y != Rows - 1) {
            os << "\n";
        }
    }
    os << "]";
    return os;
}
}  // namespace educelab
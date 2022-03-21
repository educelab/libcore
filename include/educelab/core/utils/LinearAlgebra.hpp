#pragma once

/** @file */

#include "educelab/core/utils/Math.hpp"

namespace educelab::linalg
{

/**
 * @brief Solve a system of linear equations using Cramer's rule
 *
 * @throws std::runtime_error if \f$ det(A) \approx 0 \f$
 */
template <typename MatrixType, typename VectorType>
auto solve_cramer(const MatrixType& A, const VectorType& b) -> VectorType
{
    auto detA = determinant(A);
    if (almost_zero(detA)) {
        throw std::runtime_error("Determinant of A is zero");
    }

    VectorType res;
    for (std::size_t x{0}; x < A.cols; x++) {
        auto mC = A;
        mC(0, x) = b[0];
        mC(1, x) = b[1];
        mC(2, x) = b[2];
        res[x] = determinant(mC) / detA;
    }
    return res;
}

}  // namespace educelab::linalg
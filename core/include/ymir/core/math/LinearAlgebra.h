#pragma once

#include <ymir/core/Types.h>

#include <stdexcept>
#include <cmath>

namespace ymir::math
{

// ---------------------------------------------------------------------------
// matVecProduct — A[6x6] * x[6]
// ---------------------------------------------------------------------------

inline Vector6 matVecProduct(const Matrix6x6& A, const Vector6& x, double scale = 1.0) noexcept
{
    Vector6 result{};
    for (int i = 0; i < 6; ++i)
    {
        double sum = 0.0;
        for (int j = 0; j < 6; ++j)
            sum += A[i][j] * x[j];
        result[i] = scale * sum;
    }
    return result;
}

// ---------------------------------------------------------------------------
// matVecQuadratic — A[i][j] * x[j] * |x[j]|  (quadratic drag)
// ---------------------------------------------------------------------------

inline Vector6 matVecQuadratic(const Matrix6x6& A, const Vector6& x) noexcept
{
    Vector6 result{};
    for (int i = 0; i < 6; ++i)
    {
        double sum = 0.0;
        for (int j = 0; j < 6; ++j)
            sum += A[i][j] * x[j] * std::abs(x[j]);
        result[i] = sum;
    }
    return result;
}

// ---------------------------------------------------------------------------
// invert6x6 — Gauss-Jordan elimination; throws if singular
// ---------------------------------------------------------------------------

inline Matrix6x6 invert6x6(const Matrix6x6& mat)
{
    constexpr int N = 6;
    constexpr double PIVOT_THRESHOLD = 1e-12;

    // Augmented [A | I]
    std::array<std::array<double, 12>, N> aug{};
    for (int i = 0; i < N; ++i)
    {
        for (int j = 0; j < N; ++j)
            aug[i][j] = mat[i][j];
        aug[i][N + i] = 1.0;
    }

    for (int col = 0; col < N; ++col)
    {
        double pivot = aug[col][col];
        if (std::abs(pivot) < PIVOT_THRESHOLD)
            throw std::runtime_error("ymir::math::invert6x6: matrix is singular or nearly singular");

        double inv_pivot = 1.0 / pivot;
        for (int j = 0; j < 2 * N; ++j)
            aug[col][j] *= inv_pivot;

        for (int row = 0; row < N; ++row)
        {
            if (row == col)
                continue;
            double factor = aug[row][col];
            for (int j = 0; j < 2 * N; ++j)
                aug[row][j] -= factor * aug[col][j];
        }
    }

    Matrix6x6 inv{};
    for (int i = 0; i < N; ++i)
        for (int j = 0; j < N; ++j)
            inv[i][j] = aug[i][N + j];

    return inv;
}

// ---------------------------------------------------------------------------
// matAdd — A + B
// ---------------------------------------------------------------------------

inline Matrix6x6 matAdd(const Matrix6x6& A, const Matrix6x6& B) noexcept
{
    Matrix6x6 result{};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
            result[i][j] = A[i][j] + B[i][j];
    return result;
}

// ---------------------------------------------------------------------------
// matMul — A * B
// ---------------------------------------------------------------------------

inline Matrix6x6 matMul(const Matrix6x6& A, const Matrix6x6& B) noexcept
{
    Matrix6x6 result{};
    for (int i = 0; i < 6; ++i)
        for (int j = 0; j < 6; ++j)
        {
            double sum = 0.0;
            for (int k = 0; k < 6; ++k)
                sum += A[i][k] * B[k][j];
            result[i][j] = sum;
        }
    return result;
}

} // namespace ymir::math

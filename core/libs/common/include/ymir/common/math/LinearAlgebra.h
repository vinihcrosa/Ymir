#pragma once

#include <ymir/common/Types.h>

#include <algorithm>
#include <stdexcept>
#include <cmath>
#include <vector>

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

// ---------------------------------------------------------------------------
// bilinear — 2D bilinear interpolation; clamps at all borders
// z_matrix[i][j] corresponds to (x_vec[i], y_vec[j])
// ---------------------------------------------------------------------------

inline double bilinear(
    const std::vector<double>& x_vec,
    const std::vector<double>& y_vec,
    const std::vector<std::vector<double>>& z_matrix,
    double x_query,
    double y_query) noexcept
{
    // Clamp x
    if (x_query <= x_vec.front())
    {
        // bilinear on first row → 1D in y
        double t = 0.0;
        if (y_vec.size() > 1)
        {
            if (y_query <= y_vec.front()) return z_matrix[0].front();
            if (y_query >= y_vec.back())  return z_matrix[0].back();
            auto it = std::lower_bound(y_vec.begin(), y_vec.end(), y_query);
            std::size_t j1 = static_cast<std::size_t>(it - y_vec.begin());
            std::size_t j0 = j1 - 1;
            t = (y_query - y_vec[j0]) / (y_vec[j1] - y_vec[j0]);
            return z_matrix[0][j0] * (1.0 - t) + z_matrix[0][j1] * t;
        }
        return z_matrix[0][0];
    }
    if (x_query >= x_vec.back())
    {
        std::size_t iN = x_vec.size() - 1;
        if (y_vec.size() > 1)
        {
            if (y_query <= y_vec.front()) return z_matrix[iN].front();
            if (y_query >= y_vec.back())  return z_matrix[iN].back();
            auto it = std::lower_bound(y_vec.begin(), y_vec.end(), y_query);
            std::size_t j1 = static_cast<std::size_t>(it - y_vec.begin());
            std::size_t j0 = j1 - 1;
            double t = (y_query - y_vec[j0]) / (y_vec[j1] - y_vec[j0]);
            return z_matrix[iN][j0] * (1.0 - t) + z_matrix[iN][j1] * t;
        }
        return z_matrix[iN][0];
    }

    // Find x bracket
    auto ix = std::lower_bound(x_vec.begin(), x_vec.end(), x_query);
    std::size_t i1 = static_cast<std::size_t>(ix - x_vec.begin());
    std::size_t i0 = i1 - 1;
    double tx = (x_query - x_vec[i0]) / (x_vec[i1] - x_vec[i0]);

    // Clamp y
    if (y_query <= y_vec.front())
        return z_matrix[i0].front() * (1.0 - tx) + z_matrix[i1].front() * tx;
    if (y_query >= y_vec.back())
        return z_matrix[i0].back() * (1.0 - tx) + z_matrix[i1].back() * tx;

    // Find y bracket
    auto iy = std::lower_bound(y_vec.begin(), y_vec.end(), y_query);
    std::size_t j1 = static_cast<std::size_t>(iy - y_vec.begin());
    std::size_t j0 = j1 - 1;
    double ty = (y_query - y_vec[j0]) / (y_vec[j1] - y_vec[j0]);

    double z00 = z_matrix[i0][j0];
    double z01 = z_matrix[i0][j1];
    double z10 = z_matrix[i1][j0];
    double z11 = z_matrix[i1][j1];

    return z00 * (1.0 - tx) * (1.0 - ty)
         + z01 * (1.0 - tx) *        ty
         + z10 *        tx  * (1.0 - ty)
         + z11 *        tx  *        ty;
}

} // namespace ymir::math

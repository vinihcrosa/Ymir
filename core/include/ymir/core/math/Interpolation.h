#pragma once

#include <vector>
#include <stdexcept>

namespace ymir::math
{

// ---------------------------------------------------------------------------
// linear — 1D linear interpolation with clamp at extremes
// x_vec must be monotonically increasing
// ---------------------------------------------------------------------------

inline double linear(const std::vector<double>& x_vec,
                     const std::vector<double>& y_vec,
                     double                     x_query) noexcept
{
    if (x_vec.size() < 2)
        return y_vec.empty() ? 0.0 : y_vec[0];

    if (x_query <= x_vec.front())
        return y_vec.front();

    if (x_query >= x_vec.back())
        return y_vec.back();

    // Binary search for the interval
    std::size_t lo = 0;
    std::size_t hi = x_vec.size() - 1;
    while (hi - lo > 1)
    {
        std::size_t mid = (lo + hi) / 2;
        if (x_vec[mid] <= x_query)
            lo = mid;
        else
            hi = mid;
    }

    double t = (x_query - x_vec[lo]) / (x_vec[hi] - x_vec[lo]);
    return y_vec[lo] + t * (y_vec[hi] - y_vec[lo]);
}

} // namespace ymir::math

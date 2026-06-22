#pragma once

#include <array>

namespace ymir
{

struct Forces
{
    std::array<double, 6> f{};

    [[nodiscard]] static Forces zero() noexcept { return Forces{}; }

    Forces& operator+=(const Forces& other) noexcept
    {
        for (int i = 0; i < 6; ++i)
            f[i] += other.f[i];
        return *this;
    }

    [[nodiscard]] Forces operator+(const Forces& other) const noexcept
    {
        Forces result = *this;
        result += other;
        return result;
    }

    [[nodiscard]] Forces operator-(const Forces& other) const noexcept
    {
        Forces result{};
        for (int i = 0; i < 6; ++i)
            result.f[i] = f[i] - other.f[i];
        return result;
    }

    [[nodiscard]] Forces operator*(double scale) const noexcept
    {
        Forces result{};
        for (int i = 0; i < 6; ++i)
            result.f[i] = f[i] * scale;
        return result;
    }
};

} // namespace ymir

#pragma once

namespace ymir::naval
{

struct WaveComponent
{
    double amplitude    = 0.0;  // m
    double frequency    = 0.0;  // rad/s
    double direction    = 0.0;  // rad (math convention, 0 = +x)
    double wavenumber   = 0.0;  // rad/m  (deep water: k = ω²/g)
    double phase        = 0.0;  // rad (random)
};

} // namespace ymir::naval

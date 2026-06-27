#include <ymir/world/Environment.h>

#include <cassert>

namespace ymir {

void Environment::setWind(double speed_ms, double dir_deg_naut)
{
    assert(speed_ms >= 0.0);
    wind_ = {speed_ms, dir_deg_naut};
}

void Environment::setCurrent(double speed_ms, double dir_deg_naut)
{
    assert(speed_ms >= 0.0);
    current_ = {speed_ms, dir_deg_naut};
}

void Environment::setTide(double level_m)
{
    tide_ = level_m;
}

void Environment::setWaterDepth(double depth_m)
{
    assert(depth_m > 0.0);
    waterDepth_ = depth_m;
}

void Environment::setSeaState(double Hs_m, double Tp_s, double dir_deg_naut)
{
    seaState_ = {Hs_m, Tp_s, dir_deg_naut};
}

} // namespace ymir

#pragma once

#include <ymir/physics/NavalForceModel.h>
#include <ymir/vessel/VesselConfig.h>

#include <array>
#include <vector>

namespace ymir::naval
{

class TugForces final : public NavalForceModel
{
public:
    struct TugConfig
    {
        TugMode               mode        = TugMode::PUSH;
        std::array<double, 3> position{};   // body-frame fairlead / contact point (m)
        double                angle_deg   = 0.0;   // force direction in body frame (0=fwd)
        double                bollardPull = 0.0;   // N — max static pull force
        double                speedPushK  = 1.0;   // tanh speed reduction factor

        // PULL mode table axes
        std::vector<double>              pullSpeeds;   // m/s
        std::vector<double>              pullAngles;   // deg
        std::vector<std::vector<double>> pullTable;    // [speed][angle] → N
    };

    struct Config
    {
        std::vector<TugConfig> tugs;
    };

    explicit TugForces(const Config& cfg);

    std::string name() const override { return "tug"; }

private:
    Forces computeNaval(const BodyState& state, const NavalContext& ctx) override;

    Config cfg_;
};

} // namespace ymir::naval

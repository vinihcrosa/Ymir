#pragma once

#include <vector>
#include <ymir/vessel/NavalContext.h>
#include <ymir/vessel/Thruster.h>
#include <ymir/vessel/Rudder.h>

namespace ymir::naval
{

/**
 * Replays recorded time-series commands to Thruster and Rudder actuators via
 * linear interpolation.  Primary use: validation against experimental data.
 *
 * dt and ctx are unused — the time series already encodes recorded physics.
 * The parameter list satisfies the VesselController concept required by std::visit.
 */
class PrescribedController
{
public:
    /** Monotonically increasing time stamps paired with corresponding values. */
    struct TimeSeries
    {
        std::vector<double> times;   ///< seconds, strictly increasing
        std::vector<double> values;
    };

    struct Config
    {
        std::vector<TimeSeries> thrusterRPM;      ///< one entry per thruster [RPM]
        std::vector<TimeSeries> rudderAngle_rad;  ///< one entry per rudder [rad]
    };

    explicit PrescribedController(Config cfg);

    /**
     * Interpolate each series at time t and push demands to actuators.
     *
     * If cfg has fewer series than actuators, only the leading actuators are
     * updated (std::min — no crash on size mismatch).
     */
    void update(double t, double dt, const NavalContext& ctx,
                std::vector<Thruster>& thrusters, std::vector<Rudder>& rudders);

private:
    /**
     * Linear interpolation of ts at time t using std::lower_bound.
     * Returns ts.values.front() for t < ts.times.front().
     * Returns ts.values.back()  for t > ts.times.back().
     * Interpolation error guaranteed < 1e-6 for exact table points.
     */
    static double interpolate(const TimeSeries& ts, double t) noexcept;

    Config cfg_;
};

} // namespace ymir::naval

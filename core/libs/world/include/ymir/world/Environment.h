#pragma once

#include <cassert>

namespace ymir {

/**
 * Shared environmental conditions for a simulation domain.
 *
 * Directions in nautical degrees: 0=North, 90=East, clockwise.
 * "From where" convention: currentDirectionNaut=90 means current flows from East.
 *
 * Thread safety: not thread-safe — do not call setters while World::step() is running.
 */
class Environment {
public:
    /** Set wind speed [m/s] and direction [nautical degrees]. Asserts speed_ms >= 0. */
    void setWind(double speed_ms, double dir_deg_naut);
    /** Set current speed [m/s] and direction [nautical degrees]. Asserts speed_ms >= 0. */
    void setCurrent(double speed_ms, double dir_deg_naut);
    /** Set tide level [m]. Positive = tide raises water level. */
    void setTide(double level_m);
    /** Set water depth [m]. Asserts depth_m > 0. */
    void setWaterDepth(double depth_m);
    /** Store sea state (Hs [m], Tp [s], direction [nautical deg]) for future domain use. */
    void setSeaState(double Hs_m, double Tp_s, double dir_deg_naut);

    double windSpeed()            const noexcept { return wind_.speed; }
    double windDirectionNaut()    const noexcept { return wind_.dir; }
    double currentSpeed()         const noexcept { return current_.speed; }
    double currentDirectionNaut() const noexcept { return current_.dir; }
    double waterDepth()           const noexcept { return waterDepth_; }
    double tide()                 const noexcept { return tide_; }

private:
    struct SpeedDir { double speed = 0.0; double dir = 0.0; };
    struct SeaState { double Hs = 0.0; double Tp = 0.0; double dir = 0.0; };

    SpeedDir wind_;
    SpeedDir current_;
    SeaState seaState_;
    double   waterDepth_ = 100.0;
    double   tide_       = 0.0;
};

} // namespace ymir

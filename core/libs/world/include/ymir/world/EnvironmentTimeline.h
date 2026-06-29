#pragma once

#include <string>
#include <vector>

#include <ymir/world/Environment.h>

namespace ymir {

/**
 * Owns the full environmental condition timeline for a simulation.
 *
 * Deserializes an EnvironmentProfileDTO JSON string, stores keyframe series,
 * and resolves interpolated and vectorially composed values into Environment
 * setters at each simulation step. Resolution uses shortest-path angular
 * interpolation for direction and linear interpolation for speed scalars.
 * Multiple current or wind series are composed by vector addition.
 */
class EnvironmentTimeline {
public:
    /** Single current or wind condition at a point in time. */
    struct UniformKeyframe {
        double t;       ///< Keyframe time [s]
        double speed;   ///< Condition speed [m/s], >= 0
        double dirNaut; ///< Nautical "from" direction [deg], [0, 360)
    };

    /** Single wave condition at a point in time. */
    struct WaveKeyframe {
        /** Wave spectrum type. */
        enum class Spectrum { JONSWAP, PIERSON, REGULAR };

        double   t;        ///< Keyframe time [s]
        double   Hs;       ///< Significant wave height [m], >= 0
        double   Tp;       ///< Peak period [s], > 0
        double   dirNaut;  ///< Wave direction, nautical "from" [deg]
        double   gamma;    ///< JONSWAP peak enhancement factor (default 3.3)
        Spectrum spectrum;
    };

    /**
     * Deserialize an EnvironmentProfileDTO JSON string.
     *
     * Sorts all keyframes by t on load. Throws std::runtime_error for:
     * malformed JSON, missing required fields, speed < 0, Hs < 0, Tp <= 0,
     * or invalid spectrum string.
     */
    void loadJson(const std::string& json);

    /**
     * Resolve conditions at time t and write to env setters.
     *
     * No-op when empty(). Clamps to boundary keyframe values outside the
     * timeline range. Multiple current/wind series compose by vector addition.
     * Direction interpolation uses shortest-path angular interpolation.
     */
    void advanceStep(double t, Environment& env) const;

    /** Returns true when no conditions have been loaded. */
    bool empty() const noexcept;

    /** Clear all loaded data. empty() returns true after this call. */
    void reset() noexcept;

private:
    /** Inner vector = one condition series; outer = N series composing vectorially. */
    std::vector<std::vector<UniformKeyframe>> currentSeries_;
    std::vector<std::vector<UniformKeyframe>> windSeries_;
    std::vector<WaveKeyframe>                 waveSeries_;
};

} // namespace ymir

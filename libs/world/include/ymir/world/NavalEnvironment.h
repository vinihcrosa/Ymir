#pragma once

namespace ymir::naval
{

/**
 * Mutable environmental conditions for a naval simulation.
 * Updated between steps via NavalSimulation::setEnvironment().
 *
 * Directions in nautical degrees: 0=North, 90=East, clockwise.
 * "From where" convention: currentDirectionNaut=90 means current flows from East.
 */
struct NavalEnvironment
{
    double currentSpeed         = 0.0;    // m/s
    double currentDirectionNaut = 0.0;    // deg — from where current comes
    double windSpeed            = 0.0;    // m/s
    double windDirectionNaut    = 0.0;    // deg — from where wind comes
    double waterDepth           = 100.0;  // m — positive downward; default deep water
    double tide                 = 0.0;    // m — positive = tide raises water level
};

} // namespace ymir::naval

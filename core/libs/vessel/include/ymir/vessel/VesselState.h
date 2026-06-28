#pragma once

#include <cstdint>
#include <vector>

namespace ymir::vessel {

/** Navigation light state. */
enum class NavigationLight : std::uint8_t { Off, On };

/** COLREGS shape displayed by a vessel. */
enum class ColregsShape : std::uint8_t { Ball, Cone, Cylinder, Diamond };

/** High-level operational state of a vessel per COLREGS. */
enum class OperationalState : std::uint8_t {
    Underway,
    Anchored,
    Moored,
    Aground,
    RestrictedManeuverability,
    NotUnderCommand
};

/** Set of navigation lights fitted to a vessel. */
struct NavigationLights {
    NavigationLight mast      = NavigationLight::Off;
    NavigationLight port      = NavigationLight::Off;
    NavigationLight starboard = NavigationLight::Off;
    NavigationLight stern     = NavigationLight::Off;
    NavigationLight anchor    = NavigationLight::Off;
};

/**
 * Passive snapshot of visible vessel state exposed each simulation tick.
 * No transition logic — state changes are the application's responsibility.
 */
struct VesselState {
    NavigationLights          lights;
    std::vector<ColregsShape> shapes;
    OperationalState          operationalState = OperationalState::Underway;
};

} // namespace ymir::vessel

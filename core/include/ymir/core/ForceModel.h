#pragma once

#include <ymir/core/BodyState.h>
#include <ymir/core/Forces.h>

#include <string>

namespace ymir
{

/**
 * Abstract interface for all force models.
 *
 * Implementations receive an immutable BodyState snapshot and return
 * the 6-DOF force/moment vector [Fx, Fy, Fz, Mx, My, Mz] in SI units.
 *
 * compute() is intentionally non-const: force models may maintain
 * internal state (e.g. Van der Pol oscillator, wave drift integration).
 */
class ForceModel
{
public:
    virtual ~ForceModel() = default;

    virtual Forces compute(const BodyState& state) = 0;

    virtual std::string name() const { return "unnamed"; }

    ForceModel(const ForceModel&)            = delete;
    ForceModel& operator=(const ForceModel&) = delete;

protected:
    ForceModel() = default;
};

// ---------------------------------------------------------------------------
// ZeroForceModel — always returns zero forces (used in tests)
// ---------------------------------------------------------------------------

class ZeroForceModel final : public ForceModel
{
public:
    Forces      compute(const BodyState&) override { return Forces::zero(); }
    std::string name() const override { return "zero"; }
};

} // namespace ymir

#include <ymir/vessel/DynamicVessel.h>

#include <stdexcept>

namespace ymir::naval
{

DynamicVessel::DynamicVessel(const VesselConfig&   cfg,
                             std::vector<Thruster> thrusters,
                             std::vector<Rudder>   rudders)
    : cfg_(cfg)
    , thrusters_(std::move(thrusters))
    , rudders_(std::move(rudders))
    , controller_(PrescribedController(PrescribedController::Config{}))
{}

void DynamicVessel::setController(VesselController controller)
{
    controller_ = std::move(controller);
}

void DynamicVessel::updateControl(double t, double dt, const NavalContext& ctx)
{
    std::visit([&](auto& ctrl) {
        ctrl.update(t, dt, ctx, thrusters_, rudders_);
    }, controller_);
}

void DynamicVessel::updateStates(double dt) noexcept
{
    for (auto& t : thrusters_)
        t.update(dt);
    for (auto& r : rudders_)
        r.update(dt);
}

void DynamicVessel::syncToForceModels(ThrustForces* tf, RudderForces* rf) noexcept
{
    if (tf)
    {
        for (std::size_t i = 0; i < thrusters_.size(); ++i)
            tf->setActuatorState(i, thrusters_[i].toCommand());
    }
    if (rf)
    {
        for (std::size_t i = 0; i < rudders_.size(); ++i)
            rf->setActuatorState(i, rudders_[i].toCommand());
    }
}

ymir::vessel::VesselState& DynamicVessel::vesselState() noexcept
{
    return vesselState_;
}

const ymir::vessel::VesselState& DynamicVessel::vesselState() const noexcept
{
    return vesselState_;
}

const Thruster& DynamicVessel::thruster(std::size_t idx) const
{
    if (idx >= thrusters_.size())
        throw std::out_of_range("DynamicVessel::thruster: index out of range");
    return thrusters_[idx];
}

const Rudder& DynamicVessel::rudder(std::size_t idx) const
{
    if (idx >= rudders_.size())
        throw std::out_of_range("DynamicVessel::rudder: index out of range");
    return rudders_[idx];
}

std::size_t DynamicVessel::thrusterCount() const noexcept
{
    return thrusters_.size();
}

std::size_t DynamicVessel::rudderCount() const noexcept
{
    return rudders_.size();
}

} // namespace ymir::naval

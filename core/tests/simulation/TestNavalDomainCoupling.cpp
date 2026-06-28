#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/simulation/CouplingForceModel.h>
#include <ymir/simulation/NavalDomain.h>
#include <ymir/world/World.h>
#include <ymir/physics/RigidBody6DOF.h>
#include <ymir/vessel/DynamicVessel.h>

#include <cmath>
#include <memory>
#include <utility>
#include <vector>

using Catch::Approx;

namespace {

constexpr int kShipId = 0;
constexpr int kTug1Id = 1;
constexpr int kTug2Id = 2;
constexpr double kDt = 0.125;
constexpr int kTicks = 100;

struct CoupledScenario {
    std::unique_ptr<ymir::naval::VesselConfig> config;
    std::unique_ptr<ymir::naval::DynamicVessel> vessel;
    std::unique_ptr<ymir::World> world;

    ymir::NavalDomain& domain()
    {
        return dynamic_cast<ymir::NavalDomain&>(world->domain("naval"));
    }

    const ymir::NavalDomain& domain() const
    {
        return dynamic_cast<const ymir::NavalDomain&>(world->domain("naval"));
    }
};

ymir::CvodeConfig defaultCfg()
{
    ymir::CvodeConfig cfg{};
    cfg.reltol = 1e-6;
    cfg.abstol = 1e-9;
    return cfg;
}

std::unique_ptr<ymir::RigidBody6DOF> makeBody(int id, double mass, ymir::Vector6 q)
{
    ymir::Matrix6x6 massMatrix{};
    for (int i = 0; i < 6; ++i)
        massMatrix[i][i] = mass;

    ymir::Matrix6x6 addedMass{};
    ymir::Vector6 qdot{};
    return std::make_unique<ymir::RigidBody6DOF>(id, massMatrix, addedMass, q, qdot, defaultCfg());
}

ymir::naval::BerthManeuverSystem::Config makeBsmConfig()
{
    ymir::naval::BerthManeuverSystem::Config cfg{};

    ymir::naval::BerthManeuverSystem::BerthWaypoint wp{};
    wp.x_m = 0.0;
    wp.y_m = 100.0;
    wp.targetPhase = ymir::naval::BerthManeuverSystem::Phase::Sideway;
    wp.transitionDist_m = 500.0;
    wp.headingTarget_rad = 0.0;
    cfg.waypoints.push_back(wp);

    ymir::naval::BerthManeuverSystem::TugForceConfig tug1{};
    tug1.pushForce_N = 25000.0;
    tug1.arm_m = 12.0;
    cfg.tugs.push_back(tug1);

    ymir::naval::BerthManeuverSystem::TugForceConfig tug2{};
    tug2.pushForce_N = 25000.0;
    tug2.arm_m = -12.0;
    cfg.tugs.push_back(tug2);

    cfg.lateralKp = 5000.0;
    cfg.lateralKd = 0.0;
    return cfg;
}

CoupledScenario makeScenario(bool enableCoupling)
{
    CoupledScenario s;
    s.config = std::make_unique<ymir::naval::VesselConfig>();
    s.vessel = std::make_unique<ymir::naval::DynamicVessel>(*s.config,
                                                            std::vector<ymir::naval::Thruster>{},
                                                            std::vector<ymir::naval::Rudder>{});
    s.world = std::make_unique<ymir::World>();
    s.world->couplingRegistry().addLink(kTug1Id, kShipId);
    s.world->couplingRegistry().addLink(kTug2Id, kShipId);

    auto bsm = ymir::naval::BerthManeuverSystem(makeBsmConfig());
    if (enableCoupling)
        bsm.setCouplingRegistry(&s.world->couplingRegistry(), kShipId, {kTug1Id, kTug2Id});
    s.vessel->setController(std::move(bsm));

    auto domain = std::make_unique<ymir::NavalDomain>("naval");

    ymir::Vector6 shipQ{};
    ymir::Vector6 tug1Q{};
    tug1Q[0] = -20.0;
    tug1Q[1] = 15.0;
    ymir::Vector6 tug2Q{};
    tug2Q[0] = 20.0;
    tug2Q[1] = -15.0;

    domain->addBody(kShipId, makeBody(kShipId, 1.0e6, shipQ));
    domain->addBody(kTug1Id, makeBody(kTug1Id, 2.0e5, tug1Q));
    domain->addBody(kTug2Id, makeBody(kTug2Id, 2.0e5, tug2Q));
    domain->addNavalForceModel(kShipId,
        std::make_unique<ymir::CouplingForceModel>(kShipId, s.world->couplingRegistry()));
    domain->registerVessel(kShipId, *s.vessel);

    ymir::NavalDomain* rawDomain = domain.get();
    s.world->addDomain(std::move(domain));
    rawDomain->initialize();
    return s;
}

void requireFiniteState(const ymir::BodyState& state)
{
    for (int i = 0; i < 6; ++i)
    {
        REQUIRE(std::isfinite(state.q()[i]));
        REQUIRE(std::isfinite(state.qdot()[i]));
    }
}

std::size_t bodyCount(const ymir::WorldSnapshot& snapshot)
{
    std::size_t count = 0;
    for (const auto& domain : snapshot.domains)
        count += domain.bodies.size();
    return count;
}

bool snapshotContainsBody(const ymir::WorldSnapshot& snapshot, int bodyId)
{
    for (const auto& domain : snapshot.domains)
        for (const auto& body : domain.bodies)
            if (body.id == bodyId)
                return true;
    return false;
}

} // namespace

TEST_CASE("World-driven NavalDomain coupling reaches ship sway response")
{
    auto coupled = makeScenario(true);
    auto baseline = makeScenario(false);

    for (int tick = 1; tick <= kTicks; ++tick)
    {
        coupled.world->step(kDt);
        baseline.world->step(kDt);

        const auto snapshot = coupled.world->snapshot();
        REQUIRE(snapshot.simTime == Approx(static_cast<double>(tick) * kDt).margin(1e-12));
        REQUIRE(snapshot.domains.size() == 1);
        REQUIRE(snapshot.domains[0].name == "naval");
        REQUIRE(bodyCount(snapshot) == 3);
        REQUIRE(snapshotContainsBody(snapshot, kShipId));
        REQUIRE(snapshotContainsBody(snapshot, kTug1Id));
        REQUIRE(snapshotContainsBody(snapshot, kTug2Id));
    }

    for (int id : {kShipId, kTug1Id, kTug2Id})
    {
        requireFiniteState(coupled.domain().state(id));
        requireFiniteState(baseline.domain().state(id));
    }

    const double coupledSway = coupled.domain().state(kShipId).qdot()[1];
    const double baselineSway = baseline.domain().state(kShipId).qdot()[1];
    REQUIRE(std::abs(coupledSway - baselineSway) >= 1e-3);

    const auto snapshot = coupled.world->snapshot();
    REQUIRE(bodyCount(snapshot) == 3);

    const auto ship = coupled.domain().state(kShipId).q();
    const auto tug1 = coupled.domain().state(kTug1Id).q();
    const auto tug2 = coupled.domain().state(kTug2Id).q();
    REQUIRE(std::abs(tug1[0] - ship[0]) + std::abs(tug1[1] - ship[1]) > 1e-6);
    REQUIRE(std::abs(tug2[0] - ship[0]) + std::abs(tug2[1] - ship[1]) > 1e-6);
    REQUIRE(coupled.world->time() == static_cast<double>(kTicks) * kDt);
}

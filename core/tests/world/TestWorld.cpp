#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <ymir/world/World.h>

#include <csignal>
#include <cstdlib>
#include <fcntl.h>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <type_traits>

using Catch::Approx;

namespace {

ymir::Forces makeSwayForce(double sway)
{
    ymir::Forces f = ymir::Forces::zero();
    f.f[1] = sway;
    return f;
}

class StubDomain final : public ymir::IDomain {
public:
    explicit StubDomain(std::string name) : name_(std::move(name)) {}

    void onAddedToWorld(ymir::Environment& env, ymir::CouplingRegistry& coupling) override
    {
        env_ = &env;
        coupling_ = &coupling;
        added = true;
    }

    void step(double dt) override
    {
        ++stepCount;
        lastDt = dt;
        consumedDuringStep = coupling_->consumedForce(0);
        if (writeDuringStep)
            coupling_->writeForce(1, forceToWrite);
    }

    std::vector<ymir::BodyPosition> allBodyPositions() const override
    {
        return positions;
    }

    ymir::BodyState bodyState(int id) const override
    {
        return states.at(id);
    }

    std::string name() const override
    {
        return name_;
    }

    bool added = false;
    int stepCount = 0;
    double lastDt = 0.0;
    bool writeDuringStep = false;
    ymir::Forces forceToWrite = ymir::Forces::zero();
    ymir::Forces consumedDuringStep = ymir::Forces::zero();
    std::vector<ymir::BodyPosition> positions;
    std::map<int, ymir::BodyState> states;
    ymir::Environment* env_ = nullptr;
    ymir::CouplingRegistry* coupling_ = nullptr;

private:
    std::string name_;
};

ymir::BodyState makeState(double x, double y, double z)
{
    ymir::Vector6 q{};
    q[0] = x;
    q[1] = y;
    q[2] = z;
    return ymir::BodyState(q, {}, 0.0, 0.0);
}

} // namespace

TEST_CASE("World default-constructed time is zero")
{
    ymir::World world;
    REQUIRE(world.time() == Approx(0.0));
    REQUIRE_FALSE(std::is_copy_constructible_v<ymir::World>);
    REQUIRE_FALSE(std::is_copy_assignable_v<ymir::World>);
}

TEST_CASE("World step accumulates authoritative time")
{
    ymir::World world;
    world.step(0.5);
    world.step(0.5);
    world.step(0.5);

    REQUIRE(world.time() == Approx(1.5));
}

TEST_CASE("World addDomain injects environment and coupling before storing")
{
    ymir::World world;
    auto domain = std::make_unique<StubDomain>("naval");
    auto* raw = domain.get();

    world.addDomain(std::move(domain));

    REQUIRE(raw->added);
    REQUIRE(raw->env_ == &world.environment());
    REQUIRE(raw->coupling_ == &world.couplingRegistry());
}

TEST_CASE("World domain lookup returns added domain and throws for unknown")
{
    ymir::World world;
    world.addDomain(std::make_unique<StubDomain>("naval"));

    REQUIRE(world.domain("naval").name() == "naval");
    REQUIRE_THROWS_AS(world.domain("unknown"), std::out_of_range);
}

#if !defined(NDEBUG)
TEST_CASE("World addDomain asserts on duplicate domain names")
{
    const pid_t pid = fork();
    REQUIRE(pid >= 0);

    if (pid == 0)
    {
        std::signal(SIGABRT, SIG_DFL);
        const int devNull = open("/dev/null", O_WRONLY);
        if (devNull >= 0)
        {
            dup2(devNull, STDOUT_FILENO);
            dup2(devNull, STDERR_FILENO);
            close(devNull);
        }

        ymir::World world;
        world.addDomain(std::make_unique<StubDomain>("naval"));
        world.addDomain(std::make_unique<StubDomain>("naval"));
        std::_Exit(EXIT_SUCCESS);
    }

    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFSIGNALED(status));
    REQUIRE(WTERMSIG(status) == SIGABRT);
}
#endif

TEST_CASE("World step resets stale writes before domains and resolves fresh writes after domains")
{
    ymir::World world;
    world.couplingRegistry().addLink(1, 0);
    world.couplingRegistry().writeForce(1, makeSwayForce(3.0));
    world.couplingRegistry().resolve();
    world.couplingRegistry().writeForce(1, makeSwayForce(9.0));

    auto domain = std::make_unique<StubDomain>("naval");
    auto* raw = domain.get();
    raw->writeDuringStep = true;
    raw->forceToWrite = makeSwayForce(5.0);
    world.addDomain(std::move(domain));

    world.step(0.25);

    REQUIRE(raw->stepCount == 1);
    REQUIRE(raw->lastDt == Approx(0.25));
    REQUIRE(raw->consumedDuringStep.f[1] == Approx(3.0));
    REQUIRE(world.couplingRegistry().consumedForce(0).f[1] == Approx(5.0));
}

TEST_CASE("World snapshot includes time, environment, domains, and body states")
{
    ymir::World world;
    world.environment().setWind(5.0, 90.0);

    auto domain = std::make_unique<StubDomain>("naval");
    domain->positions = {{7, 1.0, 2.0, 3.0}, {9, 4.0, 5.0, 6.0}};
    domain->states.emplace(7, makeState(1.0, 2.0, 3.0));
    domain->states.emplace(9, makeState(4.0, 5.0, 6.0));
    world.addDomain(std::move(domain));

    world.step(0.2);
    world.step(0.3);

    const auto snapshot = world.snapshot();
    REQUIRE(snapshot.simTime == Approx(world.time()));
    REQUIRE(snapshot.environment.windSpeed_ms == Approx(5.0));
    REQUIRE(snapshot.domains.size() == 1);
    REQUIRE(snapshot.domains[0].name == "naval");
    REQUIRE(snapshot.domains[0].bodies.size() == 2);
    REQUIRE(snapshot.domains[0].bodies[0].id == 7);
    REQUIRE(snapshot.domains[0].bodies[0].state.q()[0] == Approx(1.0));
    REQUIRE(snapshot.domains[0].bodies[1].id == 9);
    REQUIRE(snapshot.domains[0].bodies[1].state.q()[1] == Approx(5.0));
}

TEST_CASE("World snapshot contains one entry per added domain")
{
    ymir::World world;
    world.addDomain(std::make_unique<StubDomain>("naval"));
    world.addDomain(std::make_unique<StubDomain>("structural"));

    const auto snapshot = world.snapshot();

    REQUIRE(snapshot.domains.size() == 2);
    REQUIRE(snapshot.domains[0].name == "naval");
    REQUIRE(snapshot.domains[1].name == "structural");
}

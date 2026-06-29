#include <ymir/world/EnvironmentTimeline.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace ymir {

namespace {

constexpr double kPi = 3.14159265358979323846;

double normDeg(double deg) noexcept
{
    deg = std::fmod(deg, 360.0);
    if (deg < 0.0) deg += 360.0;
    return deg;
}

// Shortest-path angular delta from `from` to `to`, result in [-180, 180].
double shortestDelta(double from, double to) noexcept
{
    double d = to - from;
    if (d >  180.0) d -= 360.0;
    if (d < -180.0) d += 360.0;
    return d;
}

// Nautical "from" direction → math angle [rad] for velocity vector components.
// Convention: 0=North CW "from" → current flows South → math angle 270°.
double nautFromToMathRad(double dirNaut) noexcept
{
    return (270.0 - dirNaut) * kPi / 180.0;
}

// Resultant Cartesian vector (flows-to) → nautical "from" direction [deg].
double mathVecToNautFrom(double vx, double vy) noexcept
{
    double mathFromDeg = std::atan2(-vy, -vx) * 180.0 / kPi;
    return normDeg(90.0 - mathFromDeg);
}

struct InterpResult { double speed; double dirNaut; };

InterpResult interpolateUniform(
    const std::vector<EnvironmentTimeline::UniformKeyframe>& series,
    double t) noexcept
{
    if (series.empty())           return {0.0, 0.0};
    if (t <= series.front().t)    return {series.front().speed, series.front().dirNaut};
    if (t >= series.back().t)     return {series.back().speed,  series.back().dirNaut};

    std::size_t k2 = 1;
    while (k2 < series.size() && series[k2].t < t) ++k2;

    const auto& kf1 = series[k2 - 1];
    const auto& kf2 = series[k2];
    const double alpha = (t - kf1.t) / (kf2.t - kf1.t);

    const double speed = kf1.speed + alpha * (kf2.speed - kf1.speed);
    const double delta = shortestDelta(kf1.dirNaut, kf2.dirNaut);
    const double dir   = normDeg(kf1.dirNaut + alpha * delta);

    return {speed, dir};
}

// Vectorially compose N uniform series at time t → (resultSpeed, resultDirNaut).
// Returns {0, 0} when the outer vector is empty.
std::pair<double, double> composeVectorial(
    const std::vector<std::vector<EnvironmentTimeline::UniformKeyframe>>& seriesSet,
    double t)
{
    double vx = 0.0, vy = 0.0;
    for (const auto& series : seriesSet) {
        const auto [speed, dirNaut] = interpolateUniform(series, t);
        const double mathAngle = nautFromToMathRad(dirNaut);
        vx += speed * std::cos(mathAngle);
        vy += speed * std::sin(mathAngle);
    }
    const double resultSpeed = std::hypot(vx, vy);
    const double resultDir   = (resultSpeed < 1e-15) ? 0.0 : mathVecToNautFrom(vx, vy);
    return {resultSpeed, resultDir};
}

EnvironmentTimeline::WaveKeyframe::Spectrum parseSpectrum(const std::string& s)
{
    if (s == "JONSWAP")  return EnvironmentTimeline::WaveKeyframe::Spectrum::JONSWAP;
    if (s == "PIERSON")  return EnvironmentTimeline::WaveKeyframe::Spectrum::PIERSON;
    if (s == "REGULAR")  return EnvironmentTimeline::WaveKeyframe::Spectrum::REGULAR;
    throw std::runtime_error("EnvironmentTimeline: invalid spectrum '" + s + "'");
}

std::vector<EnvironmentTimeline::UniformKeyframe>
parseUniformSeries(const nlohmann::json& arr, const char* context)
{
    std::vector<EnvironmentTimeline::UniformKeyframe> kfs;
    kfs.reserve(arr.size());
    for (const auto& kf : arr) {
        if (!kf.contains("t") || !kf.contains("speed") || !kf.contains("dirNaut")) {
            throw std::runtime_error(
                std::string("EnvironmentTimeline: missing required field in ")
                + context + " keyframe (t, speed, or dirNaut)");
        }
        const double speed = kf.at("speed").get<double>();
        if (speed < 0.0)
            throw std::runtime_error("EnvironmentTimeline: speed must be >= 0");
        kfs.push_back({kf.at("t").get<double>(), speed, kf.at("dirNaut").get<double>()});
    }
    std::sort(kfs.begin(), kfs.end(),
              [](const EnvironmentTimeline::UniformKeyframe& a,
                 const EnvironmentTimeline::UniformKeyframe& b) { return a.t < b.t; });
    return kfs;
}

} // namespace

// ---------------------------------------------------------------------------

void EnvironmentTimeline::loadJson(const std::string& json)
{
    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json);
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(
            std::string("EnvironmentTimeline: malformed JSON: ") + e.what());
    }

    if (!j.contains("currentSeries") || !j.contains("windSeries") || !j.contains("waveSeries"))
        throw std::runtime_error(
            "EnvironmentTimeline: missing required top-level field "
            "(currentSeries, windSeries, or waveSeries)");

    std::vector<std::vector<UniformKeyframe>> currentSeries;
    for (const auto& s : j.at("currentSeries"))
        currentSeries.push_back(parseUniformSeries(s, "current"));

    std::vector<std::vector<UniformKeyframe>> windSeries;
    for (const auto& s : j.at("windSeries"))
        windSeries.push_back(parseUniformSeries(s, "wind"));

    std::vector<WaveKeyframe> waveSeries;
    waveSeries.reserve(j.at("waveSeries").size());
    for (const auto& kf : j.at("waveSeries")) {
        if (!kf.contains("t") || !kf.contains("Hs") || !kf.contains("Tp")
            || !kf.contains("dirNaut") || !kf.contains("spectrum"))
            throw std::runtime_error(
                "EnvironmentTimeline: missing required field in wave keyframe "
                "(t, Hs, Tp, dirNaut, or spectrum)");

        const double Hs = kf.at("Hs").get<double>();
        const double Tp = kf.at("Tp").get<double>();
        if (Hs < 0.0)  throw std::runtime_error("EnvironmentTimeline: Hs must be >= 0");
        if (Tp <= 0.0) throw std::runtime_error("EnvironmentTimeline: Tp must be > 0");

        const double gamma = kf.contains("gamma") ? kf.at("gamma").get<double>() : 3.3;
        waveSeries.push_back({
            kf.at("t").get<double>(), Hs, Tp,
            kf.at("dirNaut").get<double>(), gamma,
            parseSpectrum(kf.at("spectrum").get<std::string>())
        });
    }
    std::sort(waveSeries.begin(), waveSeries.end(),
              [](const WaveKeyframe& a, const WaveKeyframe& b) { return a.t < b.t; });

    // Commit atomically after all validation passes.
    currentSeries_ = std::move(currentSeries);
    windSeries_    = std::move(windSeries);
    waveSeries_    = std::move(waveSeries);
}

void EnvironmentTimeline::advanceStep(double t, Environment& env) const
{
    if (empty()) return;

    if (!currentSeries_.empty()) {
        auto [speed, dir] = composeVectorial(currentSeries_, t);
        env.setCurrent(speed, dir);
    }

    if (!windSeries_.empty()) {
        auto [speed, dir] = composeVectorial(windSeries_, t);
        env.setWind(speed, dir);
    }

    if (!waveSeries_.empty()) {
        double Hs, Tp, dirNaut, gamma;
        if (t <= waveSeries_.front().t) {
            const auto& kf = waveSeries_.front();
            Hs = kf.Hs; Tp = kf.Tp; dirNaut = kf.dirNaut; gamma = kf.gamma;
        } else if (t >= waveSeries_.back().t) {
            const auto& kf = waveSeries_.back();
            Hs = kf.Hs; Tp = kf.Tp; dirNaut = kf.dirNaut; gamma = kf.gamma;
        } else {
            std::size_t k2 = 1;
            while (k2 < waveSeries_.size() && waveSeries_[k2].t < t) ++k2;
            const auto& kf1 = waveSeries_[k2 - 1];
            const auto& kf2 = waveSeries_[k2];
            const double alpha = (t - kf1.t) / (kf2.t - kf1.t);
            Hs     = kf1.Hs    + alpha * (kf2.Hs    - kf1.Hs);
            Tp     = kf1.Tp    + alpha * (kf2.Tp    - kf1.Tp);
            gamma  = kf1.gamma + alpha * (kf2.gamma - kf1.gamma);
            const double delta = shortestDelta(kf1.dirNaut, kf2.dirNaut);
            dirNaut = normDeg(kf1.dirNaut + alpha * delta);
        }
        env.setSeaState(Hs, Tp, dirNaut);
        (void)gamma; // stored in WaveKeyframe; used by future wave physics
    }
}

bool EnvironmentTimeline::empty() const noexcept
{
    return currentSeries_.empty() && windSeries_.empty() && waveSeries_.empty();
}

void EnvironmentTimeline::reset() noexcept
{
    currentSeries_.clear();
    windSeries_.clear();
    waveSeries_.clear();
}

} // namespace ymir

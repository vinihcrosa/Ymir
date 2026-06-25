#pragma once

namespace ymir
{

struct CvodeConfig
{
    double reltol   = 1e-8;
    double abstol   = 1e-10;
    long   maxSteps = 10000;
    double maxStep  = 0.0;  // 0 = no limit (CVODE decides)
};

} // namespace ymir

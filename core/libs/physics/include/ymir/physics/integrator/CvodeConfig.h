#pragma once

// CvodeConfig is now an alias for RK45Config.
// Kept for backward compatibility — tests and existing code can still use CvodeConfig.
#include <ymir/physics/integrator/RK45Integrator.h>

namespace ymir
{
using CvodeConfig = RK45Config;
} // namespace ymir

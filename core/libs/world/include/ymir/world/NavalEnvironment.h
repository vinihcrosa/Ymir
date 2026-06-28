#pragma once

#include <ymir/world/Environment.h>

namespace ymir {

/** @deprecated Use ymir::Environment directly. */
using NavalEnvironment [[deprecated("Use Environment")]] = Environment;

} // namespace ymir

namespace ymir::naval {

/** @deprecated Use ymir::Environment directly. */
using NavalEnvironment [[deprecated("Use ymir::Environment")]] = ymir::Environment;

} // namespace ymir::naval

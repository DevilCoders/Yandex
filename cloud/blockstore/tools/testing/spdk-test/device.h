#pragma once

#include "private.h"

#include <cloud/blockstore/libs/spdk/public.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NCloud::NBlockStore {

////////////////////////////////////////////////////////////////////////////////

TVector<TString> RegisterDevices(
    NSpdk::ISpdkEnvPtr env,
    const TOptions& opts);

NSpdk::ISpdkTargetPtr CreateTarget(
    NSpdk::ISpdkEnvPtr env,
    const TOptions& opts,
    const TVector<TString>& devices);

}   // namespace NCloud::NBlockStore

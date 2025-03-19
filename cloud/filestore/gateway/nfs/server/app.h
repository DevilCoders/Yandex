#pragma once

#include "private.h"

namespace NCloud::NFileStore::NGateway {

////////////////////////////////////////////////////////////////////////////////

void ConfigureSignals();

int AppMain(TBootstrap& bootstrap);
void AppStop(int exitCode);

}   // namespace NCloud::NFileStore::NGateway

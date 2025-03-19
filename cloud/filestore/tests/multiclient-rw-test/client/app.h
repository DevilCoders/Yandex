#pragma once

#include "private.h"

namespace NCloud::NFileStore {

////////////////////////////////////////////////////////////////////////////////

void ConfigureSignals();

int AppMain(TOptionsPtr options);
void AppStop();

}   // namespace NCloud::NBlockStore

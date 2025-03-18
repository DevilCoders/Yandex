#pragma once

#include "inc_exc_filter.h"

#include <util/generic/strbuf.h>
#include <util/stream/buffer.h>

void ColDiff(TStringBuf v0, TStringBuf v1, const TIncExcFilter<size_t>& filter, TBufferOutput& out);

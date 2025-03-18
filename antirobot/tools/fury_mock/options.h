#pragma once

#include "path_handlers.h"

#include <library/cpp/getopt/last_getopt.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>

NLastGetopt::TOpts CreateOptions();
ui16 ParsePort(const NLastGetopt::TOptsParseResult& opts);
THashMap<TStringBuf, TPathHandler> ParseHandlers(const NLastGetopt::TOptsParseResult& opts);
TDuration ParseResponseWaitTime(const NLastGetopt::TOptsParseResult& opts);

#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/maybe.h>

TMaybe<ui64> TryParseShardTimestamp(const TStringBuf);

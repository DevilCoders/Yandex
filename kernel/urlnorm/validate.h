#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

#include <util/system/defaults.h>

bool IsValidRobotUrl(const TStringBuf& url, TString& errorDescription);

#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

// returns true if was able to decode query in utf-8, false - else (or if empty if isEmptyOk == false)
bool CheckAndFixQueryStringUTF8(TStringBuf query, TString& dstQuery, bool toLower = true, bool isEmptyOk = false);

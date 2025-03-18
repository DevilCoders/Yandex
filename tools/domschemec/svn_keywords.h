#pragma once

#include <util/generic/string.h>

bool IsSvnKeyword(const TStringBuf&);
TString ExtractSvnKeywordValue(const TStringBuf&);

#pragma once

#include <util/generic/string.h>

bool IDNAUrlToUtf8(const TString& inUrl, TString& outUrl, bool cutScheme = false);

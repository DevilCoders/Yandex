#pragma once

#include <util/system/defaults.h>
#include <util/generic/string.h>

bool IsStopWord(const TString& word);
bool IsStopWord(const TUtf16String& word);

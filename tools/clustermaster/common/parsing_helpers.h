#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>

bool ParseRange(const TString &first, const TString &last, TVector<TString> &output);

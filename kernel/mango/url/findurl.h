#pragma once

#include <util/string/vector.h>

namespace NMango {
    TVector<TString> FindUrls(const TString &text);
    TVector<TString> CutUrls(const TString &text, TVector<TString> *urls = nullptr);
    TString RemoveUrls(const TString &text);
}


#pragma once

#include <util/generic/hash.h>

class TSynsetDictWriter {
private:
    typedef TVector<TString> TSynset;
    typedef THashMap<TString, TSynset> TData;
    TData Data;

public:
    void Add(const TString& lemma, const TString& synonym);
    void Write(const TString& filename);
};

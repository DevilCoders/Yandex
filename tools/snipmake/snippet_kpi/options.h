#pragma once
#include <util/draft/date.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>

using TSnippetType2K = THashMap<TString, double>;

class TOptions {
public:
    TString ServerName;
    TString UserName;
    TDate MinDate;
    TDate MaxDate;
    TString DomainFilter;
    TString OutputFileName;
    bool SampleByUid = false;
    TString ConfigFileName;
    TSnippetType2K SnippetType2K;
    TString StatisticsFileName;
public:
    TOptions(int argc, const char* argv[]);
    void LoadCoefficients(const TString& configFileName);
    bool IsUnknownSnippetType(const TString& snippetType) const;
    double GetK(const TString& snippetType) const;
};

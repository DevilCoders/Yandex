#pragma once

#include <util/generic/set.h>
#include <util/generic/string.h>

class TUrlVariants {

public:
    TSet<TString> Urls;

    void EnhanceWithSlashesAndWWW(const TString& src);
    void ProcessUrl(TString src);
    void URLUnescapeAndProcess(const TString& src);
    void IDNAProcess(const TString& src);
private:
    void AddWithLowerCased(const TString& src);
    void Slashify(const TString& src);
};


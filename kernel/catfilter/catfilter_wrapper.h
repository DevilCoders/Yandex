#pragma once

#include <kernel/mirrors/mirrors_trie.h>

#include "catfilter.h"

#include <util/generic/vector.h>

class ICatFilter {
public:
    virtual ~ICatFilter() {}
    virtual TCatAttrsPtr Find(const TString& url) const = 0;
    virtual void Print() {
        ythrow yexception() << "Not implemented";
    }
};

ICatFilter* GetCatFilter(const TString& filename, bool map = true, TMirrorsMappedTrie* mirrors = nullptr);

class TCatFilterObj : public ICatFilter {
private:
    const TCatFilter* Filter;
    THolder<const TCatFilter> MyFilter;

public:
    TCatAttrsPtr Find(const TString& url) const override;
    TCatFilterObj(const TString& filename, bool map = true);
    TCatFilterObj(const TCatFilter* filter); // expected that Load was done and filter is not owned.
};

class TCatFilterProxy :  public ICatFilter {
private:
    const ICatFilter& CatFilter;

public:
    TCatFilterProxy(const ICatFilter& catFilter)
        : CatFilter(catFilter)
    {}

    TCatAttrsPtr Find(const TString& url) const override {
        return CatFilter.Find(url);
    }
};

#pragma once

#include <util/charset/wide.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>

namespace NSc
{
    class TValue;
}

namespace NSnippets
{
    class TConfig;
    class TArchiveView;
}

class TWordFilter;

class TEntityClassier {
private:
    struct TImpl;

    THolder<TImpl> Impl;

public:
    TEntityClassier(const NSc::TValue& entityData, const TWordFilter& wf);
    ~TEntityClassier();

    void ClassifySentence(const TUtf16String& rawSent);
    void FinalizeSent();
    void FinalizeAllStat();

    TString GetResult() const;
};

TString EntityClassify(const NSnippets::TConfig& cfg, const NSnippets::TArchiveView& view);

#pragma once

#include "base.h"

#include <kernel/remorph/tokenizer/multitoken_split.h>

#include <library/cpp/langmask/langmask.h>

#include <util/generic/string.h>

namespace NRemorphAPI {

namespace NImpl {

class TFactory: public TBase, public IFactory {
private:
    TLangMask Langs;
    TString LangsStr;
    bool DetectSentences;
    size_t MaxSentenceTokens;
    NToken::EMultitokenSplit MultitokenSplit;

    TString LastError;

public:
    TFactory();

    IProcessor* CreateProcessor(const char* protoPath) override;
    const char* GetLangs() const override;
    bool SetLangs(const char* langs) override;
    bool GetDetectSentences() const override;
    void SetDetectSentences(bool detectSentences) override;
    unsigned long GetMaxSentenceTokens() const override;
    void SetMaxSentenceTokens(unsigned long maxSentenceTokens) override;
    EMultitokenSplitMode GetMultitokenSplitMode() const override;
    bool SetMultitokenSplitMode(EMultitokenSplitMode multitokenSplitMode) override;
    const char* GetLastError() const override;
};

} // NImpl

} // NRemorphAPI

#include "factory.h"
#include "processor.h"

#include <kernel/remorph/text/textprocessor.h>
#include <kernel/remorph/tokenizer/tokenizer.h>

#include <library/cpp/charset/doccodes.h>
#include <util/generic/yexception.h>
#include <util/generic/utility.h>

namespace NRemorphAPI {

namespace NImpl {

namespace {

inline NToken::EMultitokenSplit GetImplValue(EMultitokenSplitMode multitokenSplitMode) {
    switch (multitokenSplitMode) {
        case MSM_MINIMAL:
            return NToken::MS_MINIMAL;
        case MSM_SMART:
            return NToken::MS_SMART;
        case MSM_ALL:
            return NToken::MS_ALL;
        default:
            throw yexception() << "Unsupported multitoken split mode";
    }
}

inline EMultitokenSplitMode GetApiValue(NToken::EMultitokenSplit multitokenSplit) {
    EMultitokenSplitMode multitokenSplitMode = MSM_MINIMAL;
    switch (multitokenSplit) {
        case NToken::MS_MINIMAL:
            multitokenSplitMode = MSM_MINIMAL;
            break;
        case NToken::MS_SMART:
            multitokenSplitMode = MSM_SMART;
            break;
        case NToken::MS_ALL:
            multitokenSplitMode = MSM_ALL;
            break;
        default:
            Y_ASSERT(false);
    }
    return multitokenSplitMode;
}

}

TFactory::TFactory()
    : Langs(NLanguageMasks::BasicLanguages())
    , LangsStr(NLanguageMasks::ToString(Langs))
    , DetectSentences(NToken::BD_DEFAULT)
    , MaxSentenceTokens(100u)
    , MultitokenSplit(NToken::MS_MINIMAL)
    , LastError()
{
}

IProcessor* TFactory::CreateProcessor(const char* protoPath) {
    LastError.clear();
    try {
        NText::TTextFactProcessorPtr pr(new NText::TTextFactProcessor(protoPath));
        NToken::TTokenizeOptions opts(
            DetectSentences ? NToken::BD_DEFAULT : NToken::BD_NONE,
            MultitokenSplit,
            DetectSentences ? MaxSentenceTokens : Max()
        );
        if (!DetectSentences)
            opts.Set(NToken::TF_NO_SENTENCE_SPLIT);

        //TLangMask lng = NLanguageMasks::CreateFromList(Langs);
        return new TProcessor(pr, opts, Langs);
    } catch (const yexception& e) {
        LastError = e.what();
    }
    return nullptr;
}

const char* TFactory::GetLangs() const {
    return LangsStr.data();
}

bool TFactory::SetLangs(const char* langs) {
    try {
        Langs = NLanguageMasks::CreateFromList(langs);
        LangsStr = NLanguageMasks::ToString(Langs);
    } catch (const yexception& e) {
        LastError = e.what();
        return false;
    }
    return true;
}

bool TFactory::GetDetectSentences() const {
    return DetectSentences;
}

void TFactory::SetDetectSentences(bool detectSentences) {
    DetectSentences = detectSentences;
}

unsigned long TFactory::GetMaxSentenceTokens() const {
    return MaxSentenceTokens;
}

void TFactory::SetMaxSentenceTokens(unsigned long maxSentenceTokens) {
    // LLP64 (Win64) sizes mismatch: 4->8 standard conversion.
    // Sizes match using other data models.
    MaxSentenceTokens = maxSentenceTokens;
}

EMultitokenSplitMode TFactory::GetMultitokenSplitMode() const {
    return GetApiValue(MultitokenSplit);
}

bool TFactory::SetMultitokenSplitMode(EMultitokenSplitMode multitokenSplitMode) {
    try {
        MultitokenSplit = GetImplValue(multitokenSplitMode);
    } catch (const yexception& e) {
        LastError = e.what();
        return false;
    }
    return true;
}

const char* TFactory::GetLastError() const {
    return LastError.empty() ? nullptr : LastError.data();
}

} // NImpl

} // NRemorphAPI

extern "C"
NRemorphAPI::IFactory* GetRemorphFactory() {
    return new NRemorphAPI::NImpl::TFactory();
}

#include <util/generic/string.h>

#include <kernel/lemmer/core/langcontext.h>
#include <library/cpp/langmask/langmask.h>
#include <library/cpp/token/charfilter.h>

#include <kernel/idf/idf.h>
#include <ysite/yandex/pure/pure.h>

#include "richnode.h"
#include "wordnode.h"
#include "loadfreq.h"
#include "pure_with_cached_word.h"

using namespace NSearchQuery;

static const char* const PureIsNotLoadedMessage = "Pure is not loaded.";

static double GetUpDivLoFreq(const TPureRecord& record, const TWordNode& node) {
    ui64 up = record.GetByForm(NPure::TitleCase);
    ui64 low = record.GetByForm(NPure::LowCase);
    for (ELanguage lg : node.GetLangMask()) {
        up += record.GetByForm(NPure::TitleCase, lg);
        low += record.GetByForm(NPure::LowCase, lg);
    }

    if (low || up)
        return (double) up / Max(low, (ui64) 1);
    else
        return -1;
}

static void SetUpDivLoFreq(const TPureRecord& record, TWordNode& node) {
    double upDivLoFreq = GetUpDivLoFreq(record, node);
    if (upDivLoFreq >= 0)
        node.SetUpDivLoFreq(upDivLoFreq);
}

static bool HasLang(const TLangMask& mask, ELanguage lg) {
    return mask.none() || mask.SafeTest(lg);
}

//---------------------------------------------------------------------------------------

namespace {

//----------------------------------------------------------------------------
    class TCountFreq {
    private:
        THashSet<TUtf16String> UniqueKeys;
        ui64 Res;
        const TPureWithCachedWord& Pure;
        const NPure::ECase PureCase;
    public:
        ui64 GetFreq() const {
            return Res;
        }

    protected:
        TCountFreq(const TPureWithCachedWord& pure, NPure::ECase pureCase)
            : Res(0)
            , Pure(pure)
            , PureCase(pureCase)
        {}

        ~TCountFreq() = default;

        void AddFreqByForm(const TUtf16String& w, ELanguage lang) {
            AddFreqByForm(w, lang, PureCase);
        }

        void AddFreqByForm(const TUtf16String& w, ELanguage lang, NPure::ECase pureCase) {
            if (CheckUniq(w, lang, true))
                Res += Pure.GetByForm(w, pureCase, lang);
        }

        void AddFreqByLex(const TUtf16String& w, ELanguage lang) {
            if (CheckUniq(w, lang, false))
                Res += Pure.GetByLex(w, PureCase, lang);
        }

        bool CheckLanguage(ELanguage lang) {
            return Pure.LemmatizedLanguages().SafeTest(lang);
        }
    private:
        bool CheckUniq(const TUtf16String& w, ELanguage lang, bool form) {
            TUtf16String key = w;
            key.push_back(' ');
            key.push_back(wchar16(lang));
            key.push_back(form ? '1' : '0');
            if (UniqueKeys.contains(key))
                return false;
            UniqueKeys.insert(key);
            return true;
        }
    };

    class TCountByForms: public TCountFreq {
        bool ExactOnly;
    public:
        TCountByForms(const TPureWithCachedWord& pure, NPure::ECase caseFlags, bool exactOnly)
            : TCountFreq(pure, caseFlags)
            , ExactOnly(exactOnly)
        {}

        void AddLemma(const TLemmaForms& lm) {
            if (!lm.FormsGenerated())
                return;
            for (TLemmaForms::TFormMap::const_iterator it = lm.GetForms().begin(); it != lm.GetForms().end(); ++it) {
                if (ExactOnly && !it->second.IsExact)
                    continue;
                AddFreqByForm(it->first, lm.GetLanguage());
            }
        }

        void AddUnknownFreq(const TUtf16String& form) {
            AddFreqByForm(form, LANG_UNK);
        }
    };

    class TCountByLemmas: public TCountFreq {
    public:
        explicit TCountByLemmas(const TPureWithCachedWord& pure)
            : TCountFreq(pure, NPure::LowCase)
        {}

        void AddLemma(const TLemmaForms& lm) {
            if (!lm.GetLanguage() || CheckLanguage(lm.GetLanguage()))
                AddFreqByLex(lm.GetLemma(), lm.GetLanguage());
            else
                AddFreqByForm(lm.GetNormalizedForm(), lm.GetLanguage());
        }

        void AddUnknownFreq(const TUtf16String& form) {
            AddFreqByForm(form, LANG_UNK, NPure::AllCase);
        }
    };
} // namespace

//------------------------------------------------------------------------------------------------------------

static ui64 NotAWordFreq(const TPureWithCachedWord& pure, const TWordNode& node) {
    if (node.GetFormType() == fExactWord || !node.NumLemmas()) {
        return pure.GetRecord().GetByForm(NPure::LowCase);
    } else {
        return pure.GetByLex(NormalizeUnicode(node.GetLemmas()[0].GetLemma()), NPure::LowCase);
    }
}

ui64 ExactWordFreq(const TPureWithCachedWord& pure, const TWordNode& node) {
    NPure::ECase pureCase = (node.GetCaseFlags() & CC_TITLECASE) ? NPure::TitleCase : NPure::AllCase;
    TCountByForms meter(pure, pureCase, true);

    for (TWordNode::TLemmasVector::const_iterator i = node.GetLemmas().begin(), mi = node.GetLemmas().end(); i != mi; ++i) {
        if (HasLang(node.GetLangMask(), i->GetLanguage()))
            meter.AddLemma(*i);
    }
    if (node.GetLangMask().none())
        meter.AddUnknownFreq(node.GetNormalizedForm());
    return meter.GetFreq();
}

ui64 CommonWordFreq(const TPureWithCachedWord& pure, const TWordNode& node, bool exactLemma) {
    Y_ASSERT(node.IsLemmerWord());

    TCountByLemmas meterLms(pure);
    TCountByForms meterExact(pure, NPure::AllCase, true);
    TCountByForms meterFix(pure, NPure::AllCase, false);

    for (TWordNode::TLemmasVector::const_iterator i = node.GetLemmas().begin(), mi = node.GetLemmas().end(); i != mi; ++i) {
        if (!HasLang(node.GetLangMask(), i->GetLanguage()))
            continue;
        if (exactLemma && !i->IsExactLemma())
            continue;

        if (i->IsOverridden() || (i->GetQuality() & TYandexLemma::QFix)) {
            meterFix.AddLemma(*i);
        } else if (i->GetQuality() & TYandexLemma::QDisabled) {
            meterExact.AddLemma(*i);
        } else {
            meterExact.AddLemma(*i);
            meterLms.AddLemma(*i);
        }
    }
    if (!node.GetLangMask().none()) {
        meterLms.AddUnknownFreq(node.GetNormalizedForm());
        meterExact.AddUnknownFreq(node.GetNormalizedForm());
    }

    ui64 freq = meterLms.GetFreq();
    if (freq < meterExact.GetFreq())
        freq = meterExact.GetFreq();

    return freq + meterFix.GetFreq();
}

static i64 GetTermCountForNode(const TPureWithCachedWord& cache, const TWordNode& node) {
    ui64 res = 0;

    if (!node.IsLemmerWord()) {
        res = NotAWordFreq(cache, node);
    } else if (node.GetFormType() == fExactWord) {
        res = ExactWordFreq(cache, node);
    } else if (node.GetFormType() == fExactLemma) {
        res = CommonWordFreq(cache, node, true);
        if (!res)
            res = ExactWordFreq(cache, node);
    } else {
        res = CommonWordFreq(cache, node, false);
    }

    return res;
}

i64 GetRevFreqForNode(const TPure& pure, const TWordNode& node) {
    TPureWithCachedWord cache(pure, node.GetNormalizedForm());

    i64 res = GetTermCountForNode(cache, node);
    return static_cast<long>(TermCount2RevFreq(pure.GetCollectionLength(), res));
}

double GetUpDivLoFreq(const TPure& pure, const TWordNode& node) {
    TPureWithCachedWord cache(pure, node.GetNormalizedForm());
    return GetUpDivLoFreq(cache.GetRecord(), node);
}

void LoadFreq(const TPure& pure, TWordNode& node) {
    if (!pure.Loaded()) {
        ythrow yexception() << PureIsNotLoadedMessage;
    }

    TPureWithCachedWord cache(pure, node.GetNormalizedForm());
    SetUpDivLoFreq(cache.GetRecord(), node);

    i64 res = GetTermCountForNode(cache, node);
    node.SetRevFr(static_cast<long>(TermCount2RevFreq(pure.GetCollectionLength(), res)));
}

void LoadFreq(const TPure& pure, TRichRequestNode& node, bool force) {
    if (!pure.Loaded()) {
        ythrow yexception() << PureIsNotLoadedMessage;
    }
    bool reload = (node.ReverseFreq < 0 || force);

    if (reload && node.WordInfo.Get() != nullptr) {
        if (IsWordOrMultitoken(node) || IsAndOp(node)) {
            TWordNode& wordNode = *node.WordInfo;
            LoadFreq(pure, wordNode);
            node.ReverseFreq = wordNode.GetRevFr();

            // TODELETE legacy emulation
            wordNode.SetRevFr(-1);
        } else if (IsAttribute(node)) {
            node.ReverseFreq = static_cast<i32>(TermCount2RevFreq(pure.GetCollectionLength(), 0)); // TODELETE
        }
    }

    for (size_t i = 0; i < node.Children.size(); ++i) {
        LoadFreq(pure, *node.Children[i], force);
    }

    for (TForwardMarkupIterator<TSynonym, false> i(node.MutableMarkup()); !i.AtEnd(); ++i) {
        LoadFreq(pure, *(i.GetData().SubTree), force);
    }

    for (TForwardMarkupIterator<TTechnicalSynonym, false> i(node.MutableMarkup()); !i.AtEnd(); ++i) {
        LoadFreq(pure, *(i.GetData().SubTree), force);
    }

    for (size_t i = 0; i < node.MiscOps.size(); ++i) {
        LoadFreq(pure, *node.MiscOps[i], force);
    }
}

i64 GetRevFreq(const TPure& pure, const TUtf16String& word, const TLanguageContext& lang, TFormType ftype) {
    if (!pure.Loaded()) {
        ythrow yexception() << PureIsNotLoadedMessage;
    }
    THolder<TWordNode> wordInfo(TWordNode::CreateLemmerNode(word, TCharSpan(0, word.length()), ftype, lang));
    LoadFreq(pure, *wordInfo);
    return wordInfo->GetRevFr();
}

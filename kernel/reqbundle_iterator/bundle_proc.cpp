#include "bundle_proc.h"
#include "position.h"

#include <ysite/yandex/posfilter/formfilter.h>
#include <library/cpp/token/charfilter.h>

#include <util/generic/cast.h>

using namespace NReqBundleIterator;

namespace NReqBundleIteratorImpl {
    TLemmBundleData::TLemmBundleData(
        NReqBundle::TConstWordAcc word,
        size_t lemmId,
        TFormRange formRange,
        TMemoryPool& pool)
        : IsStopWord(word.IsStopWord())
        , IsAttribute(word.GetLemma(lemmId).IsAttribute())
        , NlpType(word.GetNlpType())
        , DefaultLemmId(lemmId)
        , DefaultFormRange(formRange)
        , Form2LemmId(&pool)
    {
        Y_ASSERT(formRange.first <= formRange.second);
        Form2LemmId.reserve(formRange.second - formRange.first);

        if (IsAttribute) {
            // empty FormClassifier
            Y_ENSURE(word.GetLemma(lemmId).GetNumForms() == 0);
        } else if (NlpType == NLP_WORD) {
            FormClassifier = CreateFormSelector(
                word.GetWideText(), /* form */
                word.GetCaseFlags(), /* caseFlags */
                fGeneral, /* formType */
                true /* useLang */
            );
        } else {
            TAutoPtr<TWordNode> node;
            // FIXME: move this logic to kernel/reqbundle
            if (NlpType == NLP_INTEGER) {
                // let CreateAttributeIntegerNode care about "3" -> "0...03" conversion
                node = TWordNode::CreateAttributeIntegerNode(word.GetWideText(), fGeneral);
            } else {
                node = TWordNode::CreateNonLemmerNode(word.GetWideText(), fGeneral);
            }
            FormClassifier = CreateNonLemmerFormSelector(node.Get());
        }
    }

    void TLemmBundleData::AddRichTreeLemma(
        NReqBundle::TConstWordAcc word,
        size_t lemmId,
        TFormRange formRange,
        TMemoryPool& pool)
    {
        if (lemmId != DefaultLemmId && Form2LemmId.empty()) {
            // lazy-initialize Form2LemmId if there are
            // at least two lemmas with same text
            auto defLemma = word.GetLemma(DefaultLemmId);
            for (size_t formId = DefaultFormRange.first; formId < DefaultFormRange.second; formId++) {
                const TUtf16String& formText = defLemma.GetForm(formId).GetWideText();
                Form2LemmId.emplace(
                    TFormWithLang{
                        pool.AppendString<TUtf16String::char_type>(formText),
                        defLemma.GetLanguage()},
                    DefaultLemmId);
            }
        }

        const auto lemma = word.GetLemma(lemmId);
        for (size_t formId = formRange.first; formId < formRange.second; formId++) {
            const auto form = lemma.GetForm(formId);
            const TUtf16String& formText = form.GetWideText();
            if (lemmId != DefaultLemmId) {
                Form2LemmId.emplace(
                    TFormWithLang{
                        pool.AppendString<TUtf16String::char_type>(formText),
                        lemma.GetLanguage()},
                    lemmId);
            }
            if (NlpType == NLP_WORD) {
                Y_ASSERT(FormClassifier);
                if (FormClassifier) {
                    FormClassifier->AddForma(formText, lemma.GetLanguage(), form.IsExact());
                }
            }
        }
    }

    inline TWordBundleData::TWordBundleData(
        const NReqBundle::TConstWordAcc word,
        TMemoryPool& pool,
        const TGlobalOptions& globalOptions)
        : Form2Id(word, pool)
        , Lemm2Data(&pool)
        , AnyWord(word.IsAnyWord())
    {
        const bool isInteger = (word.GetNlpType() == NLP_INTEGER);

        Y_ENSURE(isInteger || word.GetNumLemmas() || AnyWord,
            "no lemmas in qbundle node");
        Y_ENSURE(word.GetNumLemmas() <= TLemmId::MaxValue,
            "too many lemmas in qbundle node");

        Lemm2Data.reserve(word.GetNumLemmas());

        for (const size_t lemmIdx : xrange(word.GetNumLemmas())) {
            const auto lemma = word.GetLemma(lemmIdx);
            const bool shouldFlatten = !isInteger && !lemma.IsAttribute()
                && !globalOptions.LemmatizedLanguages.SafeTest(lemma.GetLanguage())
                && lemma.GetNumForms() > 0;
            const bool useRawKey = lemma.IsAttribute() || isInteger;
            for (ui32 formId = 0; formId < (shouldFlatten ? lemma.GetNumForms() : 1); formId++) {
                TLemmBundleData::TFormRange formRange = shouldFlatten ?
                    TLemmBundleData::TFormRange(formId, formId + 1) :
                    TLemmBundleData::TFormRange(0, lemma.GetNumForms());
                const TUtf16String& rawKey = shouldFlatten ? lemma.GetForm(formId).GetWideText() : lemma.GetWideText();
                const TUtf16String usedKey = useRawKey ? rawKey : NormalizeUnicode(rawKey);

                auto* data = Lemm2Data.FindPtr(usedKey);
                if (!data) {
                    const auto result = Lemm2Data.emplace(
                        pool.AppendString<wchar16>(usedKey),
                        pool.New<TLemmBundleData>(word, lemmIdx, formRange, pool));

                    Y_ASSERT(result.second);
                    data = &(result.first->second);
                }
                Y_ASSERT(data);
                (*data)->AddRichTreeLemma(word, lemmIdx, formRange, pool);
            }
        }
    }

    TBlockBundleData::TBlockBundleData(
        NReqBundle::TConstBlockAcc block,
        TMemoryPool& pool,
        const TGlobalOptions& globalOptions)
        : NumWords(block.GetNumWords())
        , Words(&pool)
    {
        Distance = block.GetDistance();
        Type = block.GetType();
        Words.reserve(block.GetNumWords());

        for (size_t i = 0; i < block.GetNumWords(); i++) {
            Words.emplace_back(block.GetWord(i), pool, globalOptions);
        }
    }
} // NReqBundleIteratorImpl

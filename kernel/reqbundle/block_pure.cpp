#include "block_pure.h"
#include "sequence_accessors.h"

#include <ysite/yandex/pure/pure_container.h>
#include <kernel/idf/idf.h>
#include <util/generic/utility.h>

namespace {
    void LoadRevFreqsFromPureImpl(NReqBundle::TWordAcc word, const TPure& pure, NLingBoost::EWordFreqType type)
    {
        if (!pure.Loaded()) {
            return;
        }
        const TLangMask formLanguages = word.GetLangMask();

        ui64 formFreq = 0;
        if (!formLanguages) {
            formFreq = pure.GetByForm(word.GetWideText(), NPure::AllCase, LANG_UNK).GetFreq();
        } else {
            for (ELanguage language : formLanguages) {
                formFreq = ::Max(formFreq, pure.GetByForm(word.GetWideText(), NPure::AllCase, language).GetFreq());
            }
        }
        const i64 formRevFreq = TermCount2RevFreq(pure.GetCollectionLength(), formFreq);
        word.SetRevFreq(type, NLingBoost::CanonizeRevFreq(formRevFreq));

        ui64 maxLemmFreq = 0;
        for (auto lemma : word.Lemmas()) {
            ui64 lemmFreq = pure.GetByLex(lemma.GetWideText(), NPure::AllCase, lemma.GetLanguage()).GetFreq();
            maxLemmFreq = Max(maxLemmFreq, lemmFreq);
            lemma.SetRevFreq(type, TermCount2RevFreq(pure.GetCollectionLength(), lemmFreq));
        }
        const i64 lemmRevFreq = TermCount2RevFreq(pure.GetCollectionLength(), maxLemmFreq);
        word.SetRevFreqAllForms(type, NLingBoost::CanonizeRevFreq(lemmRevFreq));
    }
}

namespace NReqBundle {
    i64 CalcCompoundRevFreq(TConstBlockAcc block, NLingBoost::EWordFreqType type)
    {
        // treat AND(w1,...,wn) as the least frequent word from wi
        i64 maxRevFreq = NLingBoost::InvalidRevFreq;
        for (auto word : block.GetWords()) {
            maxRevFreq = ::Max<i64>(maxRevFreq, word.GetRevFreqAllForms(type));
        }
        Y_ASSERT(NLingBoost::InvalidRevFreq == maxRevFreq || NLingBoost::IsValidRevFreq(maxRevFreq));
        return NLingBoost::CanonizeRevFreq(maxRevFreq);
    }

    void LoadRevFreqsFromPure(TBlockAcc block, const TPureContainer& container)
    {
        for (auto word : block.Words()) {
            LoadRevFreqsFromPureImpl(word, container.Get(word.GetLangMask()), NLingBoost::TRevFreq::Default);

            // Per-stream frequencies, see search/idx_term/tools/purifier/lib/common.cpp
            //
            LoadRevFreqsFromPureImpl(word, container.GetByName("stream.url"), NLingBoost::TRevFreq::Url);
            LoadRevFreqsFromPureImpl(word, container.GetByName("stream.title"), NLingBoost::TRevFreq::Title);
            LoadRevFreqsFromPureImpl(word, container.GetByName("stream.body"), NLingBoost::TRevFreq::Body);
            LoadRevFreqsFromPureImpl(word, container.GetByName("stream.all_lnk"), NLingBoost::TRevFreq::Links);
            LoadRevFreqsFromPureImpl(word, container.GetByName("stream.all_ann"), NLingBoost::TRevFreq::Ann);
        }
    }

    void LoadRevFreqsFromPure(TSequenceAcc sequence, const TPureContainer& container)
    {
        for (auto elem : sequence.Elems()) {
            if (elem.HasBlock()) {
                LoadRevFreqsFromPure(elem.Block(), container);
            }
        }
    }
} // NReqBundle

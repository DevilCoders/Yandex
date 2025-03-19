#include "disamber.h"
#include <kernel/indexer/direct_text/dt.h>
#include <util/generic/utility.h>

namespace NIndexerCore {
namespace NIndexerCorePrivate {

bool HasAmbigulation(const TDirectTextEntry2& entry) {
    if (entry.LemmatizedTokenCount < 2) // there is no ambigulation
        return false;
    bool hasAmbigulation = false;
    const TLemmatizedToken& firstLemTok = entry.LemmatizedToken[0];
    for (size_t i = 1; i < entry.LemmatizedTokenCount; i++) {
        const TLemmatizedToken& lemTok = entry.LemmatizedToken[i];
        if (lemTok.FormOffset != firstLemTok.FormOffset)
            return false; // Multitokens were skipped in the old version
        if (lemTok.Lang != firstLemTok.Lang || strcmp(lemTok.FormaText, firstLemTok.FormaText) != 0)
            return false; // entries with many forms were skipped in the old version, TODO: don't skip?
        if (strcmp(lemTok.LemmaText, firstLemTok.LemmaText) != 0)
            hasAmbigulation = true;
    }
    return hasAmbigulation;
}

static
size_t DescribeContext(TDisambModel<DisambText>& disambModel, const TDirectTextEntry2* entries, size_t start, size_t end, size_t curIndex) {
    size_t addedCount = 0;
    bool bAddedEndOfSent = false;
    ui16 curBreak = GetBreak(entries[curIndex].Posting);
    for (size_t i = start; i < end; i++) {
        if (curBreak == GetBreak(entries[i].Posting) && entries[i].LemmatizedTokenCount > 0)  {
            TWordFormAndLemmas f;
            f.LowerWordForm = entries[i].LemmatizedToken[0].FormaText;
            disambModel.AddWord(f);
            addedCount++;
            if (entries[i].SpaceCount > 0) {
                const TDirectTextSpace& textSpace = entries[i].Spaces[0];
                if (textSpace.Space && TZelDisambModel::IsModelPunctuation(WideCharToYandex.Tr(textSpace.Space[0]))) {
                    disambModel.AddWord(TWordFormAndLemmas());
                    addedCount++;
                }
            }
        } else {
            if (!bAddedEndOfSent) {
                TWordFormAndLemmas f;
                f.LowerWordForm = EndOfSentenceMarker;
                disambModel.AddWord(f);
                addedCount++;
                bAddedEndOfSent = true;
            }
        }
    }
    return addedCount;
}
}

void TDefaultDisamber::ProcessText(const TDirectTextEntry2* entries, size_t entCount, TVector<TDisambMask>* masks) {
    const static float desiredConfidentLevel = 0.8f;
    masks->resize(entCount);

    for (size_t entIndex = 0; entIndex < entCount; ++entIndex) {
        const TDirectTextEntry2& entry = entries[entIndex];
        if (!NIndexerCorePrivate::HasAmbigulation(entry))
            continue;
        if (DisambModel.GetDictionaryHomonymSignificance(entry.LemmatizedToken[0].FormaText) == -1)
            continue;

        DisambModel.DeleteWords();

        // left context
        size_t start = entIndex >= (size_t)ContextRadius ? entIndex - (size_t)ContextRadius : 0;
        size_t wordPos = NIndexerCorePrivate::DescribeContext(DisambModel, entries, start, entIndex, entIndex);

        // word itself
        {
            TWordFormAndLemmas f;
            f.LowerWordForm = entry.LemmatizedToken[0].FormaText;
            ui32 lemmCount = Min((ui32)31, entry.LemmatizedTokenCount);
            for (ui32 i = 0; i < lemmCount; i++) { // here we push back without a call of uniq() for lemma text, is it right?
                f.Lemmas.push_back(entry.LemmatizedToken[i].LemmaText);
            }
            DisambModel.AddWord(f);
        }

        // right context
        NIndexerCorePrivate::DescribeContext(DisambModel, entries, entIndex + 1,  Min(entCount, entIndex + ContextRadius + 1), entIndex);

        TDisambMask& m = (*masks)[entIndex];
        TDisambMask::TMask tMask;
        m.BestLemma = DisambModel.ApplyDisambForWord(wordPos, tMask, m.Weight);
        if (m.Weight > desiredConfidentLevel && m.BestLemma != -1)
            m.Mask = tMask.Flip();
    }
}

}

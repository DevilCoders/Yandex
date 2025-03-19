#include "richnode_helpers.h"

#include <kernel/qtree/richrequest/richnode.h>
#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <kernel/qtree/richrequest/serialization/serializer.h>
#include <kernel/qtree/richrequest/markup/ontotypemarkup.h>

#include <library/cpp/langmask/serialization/langmask.h>

using namespace NSearchQuery;

namespace {
    ui64 SingleWordMask(ui32 wordNum) {
        return wordNum < 64 ? (ui64(1) << wordNum) : 0;
    }
}

namespace NReqBundle {
namespace NDetail {
    class TBadPhraseError : public yexception {};

    void TRichNodeUnpackHelper::Unpack(const TRichRequestNode& node,
        const TLangMask& mask)
    {
        LangMask = mask;
        MainParts.clear();
        MarkupParts.clear();
        ConstraintParts.clear();
        Indexes.clear();
        TrIteratorMainPartsWordMasks.clear();
        TrIteratorMarkupPartsBestFormClasses.clear();
        TrIteratorWordCount = 0;
        QuotedConstraintInfos.clear();
        UnpackWordOrPhrase(node, 0, ENodeParsingType::TopLevelNode);
        Indexes.clear();

        if (Opts.WordOptions.StripLemmaForms) {
            for (auto& part : MainParts) {
                part.Block->StripAllForms();
            }
            for (auto& part : MarkupParts) {
                part.Block->StripAllForms();
            }
        }
    }

    void TRichNodeUnpackHelper::ResetIndexes(size_t begin, size_t end, size_t wordIndex)
    {
        for (size_t i = begin; i != end; ++i) {
            Indexes[i] = wordIndex;
        }
    }

    size_t TRichNodeUnpackHelper::GetPartIndexLowerBound(size_t begin, size_t end, size_t wordIndex) const
    {
        size_t i = begin;
        while (i < end && Indexes[i] < wordIndex) {
            ++i;
        }
        return i;
    }

    TRichNodeUnpackHelper::TBeginEnd TRichNodeUnpackHelper::UnpackWordOrPhrase(const TRichRequestNode& node, size_t wordIndex, ENodeParsingType parsingType)
    {
        Y_ASSERT(Opts.UnpackAndBlocks || parsingType != CollectSubtreeWordsForAndBlock);
        // Cerr << "-D- Unpack word or phrase at " << wordIndex << Endl;

        if (parsingType == ENodeParsingType::TopLevelNode && Opts.UnpackConstraints) {
            // Cerr << "-D- Unpack constraint" << Endl;
            UnpackConstraints(node);
        }

        if (node.Children.size() == 0) {
            // Cerr << "-D- Word " << node.GetText() << Endl;

            size_t partsBegin = MainParts.size();

            if (IsAttribute(node)) {
                if (!Opts.UnpackAttributes) {
                    return TBeginEnd(partsBegin, partsBegin);
                }
                static TUtf16String TelFullAttrName = u"tel_full";
                static TUtf16String TelLocalAttrName = u"tel_local";
                if (Opts.FilterOffBadAttributes && node.GetTextName() != TelFullAttrName && node.GetTextName() != TelLocalAttrName) {
                    return TBeginEnd(partsBegin, partsBegin);
                }
            } else if (node.WordInfo && node.WordInfo->GetLemmas().size() == 0) {
                // Cerr << "-D- Skip word " << node.GetText() << " as it has no lemmas" << Endl;
                return TBeginEnd(partsBegin, partsBegin); // Skip nodes that have no lemmas, to avoid exception in reqbundle_iterator
            }

            if (Opts.UnpackQuotedConstraint && node.IsQuoted()) {
                UnpackQuotedWordOrPhrase(QuotedConstraintInfos.emplace_back(), node);
            }

            if (parsingType != CollectSubtreeWordsForAndBlock) {
                TrIteratorMainPartsWordMasks.push_back(SingleWordMask(TrIteratorWordCount));
            }

            if (parsingType == TopLevelNode) {
                Y_ASSERT(TrIteratorWordCount == 0);
                TrIteratorWordCount = 1;
            }

            MainParts.push_back(MakeSimpleWordsOrAttributePart(node));

            Indexes.push_back(0);
            if (parsingType != CollectSubtreeWordsForAndBlock && Opts.UnpackSynonyms) {
                UnpackSynonyms(node, partsBegin, partsBegin + 1, node.IsQuoted() && Opts.UnpackQuotedConstraint);
            }
            Indexes.back() = wordIndex;

            return TBeginEnd(partsBegin, partsBegin + 1);
        }
        else if (IsMultitoken(node)) { // Does it ever work? Maybe put Y_ASSERT here.
            // Cerr << "-D- Multitoken" << Endl;
            TBeginEnd phraseRange = UnpackPhrase(node, wordIndex, parsingType);
            Y_ASSERT(phraseRange.first < phraseRange.second);

            for (size_t i = phraseRange.first; i + 1 < phraseRange.second; ++i) {
                MainParts[i].IsMultitokenFragment = true;
            }

            return phraseRange;
        }
        else {
            // Cerr << "-D- Phrase" << Endl;
            return UnpackPhrase(node, wordIndex, parsingType);
        }
    }

    void TRichNodeUnpackHelper::UnpackQuotedWordOrPhrase(
        TQuotedConstraintInfo& currentQuotedConstraint,
        const TRichRequestNode& node)
    {
        if (node.WordInfo && !node.WordInfo->GetLemmas().empty()) {

            ConstraintParts.emplace_back(MakeSimpleWordsOrAttributePart(node));
            ConstraintParts.back().ConstraintType = NLingBoost::TConstraint::Quoted;

            currentQuotedConstraint.emplace_back(
                ConstraintParts.size() - 1,
                ConstraintParts.size()
            );
        }
        size_t numChildren = node.Children.size();
        for (size_t wordInPhrase = 0; wordInPhrase < numChildren; wordInPhrase++) {
            /*
                unpacking before the proxes
            */

            UnpackQuotedWordOrPhrase(currentQuotedConstraint, *node.Children[wordInPhrase]);

            /*
                !currentQuotedConstraint.empty() as start in the begining does not matter
                (we do believe that before and after the quoted phrase there is a word, do we? :))
                It's actually vital for the ConstraintByFirstBlock
            */
            if (!currentQuotedConstraint.empty() && wordInPhrase + 1 < numChildren) {
                TProximity prox = node.Children.ProxAfter(wordInPhrase);
                if (prox.Level == BREAK_LEVEL) {
                    size_t anyWordsCount = 0;
                    if (NReqBundle::NDetail::IsAnyWordsProximity(prox, anyWordsCount)) {
                        TBlockPtr block = new TBlock;
                        auto& blockContents = BackdoorAccess(*block);
                        blockContents.Words.emplace_back();
                        blockContents.Words.back().AnyWord = true;
                        blockContents.Type = EBlockType::ExactOrdered;
                        TPart anyEmptyPart(block);
                        for (size_t i = 0; i < anyWordsCount; ++i) {
                            ConstraintParts.push_back(anyEmptyPart);
                            ConstraintParts.back().ConstraintType = NLingBoost::TConstraint::Quoted;
                            currentQuotedConstraint.emplace_back(
                                ConstraintParts.size() - 1,
                                ConstraintParts.size()
                            );
                        }
                    }
                }
            }
        }
    }


    TRichNodeUnpackHelper::TBeginEnd TRichNodeUnpackHelper::UnpackPhrase(const TRichRequestNode& node, size_t wordIndex, ENodeParsingType parsingType)
    {
        Y_ASSERT(Opts.UnpackAndBlocks || parsingType != CollectSubtreeWordsForAndBlock);
        // Cerr << "-D- Phrase len " << node.Children.size() << Endl;

        if (Opts.UnpackQuotedConstraint && IsAndOp(node) && node.IsQuoted()) {
            UnpackQuotedWordOrPhrase(QuotedConstraintInfos.emplace_back(), node);
        }

        size_t partsBegin = MainParts.size();

        size_t wordIndexInPhrase = 0;
        ENodeParsingType childParsingType = (parsingType == CollectSubtreeWordsForAndBlock ? CollectSubtreeWordsForAndBlock : OtherParsingType);
        for (; wordIndexInPhrase != node.Children.size(); ++wordIndexInPhrase) {
            TBeginEnd wordOrPhraseRange = UnpackWordOrPhrase(*node.Children[wordIndexInPhrase], wordIndexInPhrase, childParsingType);

            if (wordOrPhraseRange.second > wordOrPhraseRange.first) {
                if (wordIndexInPhrase + 1 < node.Children.size()) {
                    MainParts.back().ProxAfter = node.Children.ProxAfter(wordIndexInPhrase);
                }

                if (node.Children[wordIndexInPhrase]->WordInfo) {
                    node.Children[wordIndexInPhrase]->WordInfo->IsStopWord(MainParts.back().Stickiness);
                }
            }
            if (parsingType == TopLevelNode && IsAndOp(node)) {
                ++TrIteratorWordCount;
            }
        }
        if (parsingType == TopLevelNode && !IsAndOp(node)) {
            Y_ASSERT(TrIteratorWordCount == 0);
            TrIteratorWordCount = (node.Children.size() > 0 ? 1 : 0);
        }

        size_t partsEnd = MainParts.size();

        if (parsingType != CollectSubtreeWordsForAndBlock && Opts.UnpackSynonyms) {
            UnpackSynonyms(node, partsBegin, partsEnd, Opts.UnpackQuotedConstraint && IsAndOp(node) && node.IsQuoted());
        }
        if (parsingType != CollectSubtreeWordsForAndBlock && Opts.UnpackWares) {
            UnpackWares(node, partsBegin, partsEnd);
        }

        ResetIndexes(partsBegin, partsEnd, wordIndex);
        return TBeginEnd(partsBegin, partsEnd);
    }

    void TRichNodeUnpackHelper::UnpackConstraints(const TRichRequestNode &node)
    {
        for (const auto& child : node.MiscOps) {
            if (child->IsAndNotOp() && child->Children.size() == 1) {
                TRichNodePtr wordNode = child->Children[0];
                if (!IsAttribute(*wordNode) && wordNode->Children.size() == 0 && !(wordNode->WordInfo && wordNode->WordInfo->GetLemmas().size() == 0)) {
                    ConstraintParts.push_back(MakeSimpleWordsOrAttributePart(*wordNode));
                    ConstraintParts.back().ConstraintType = EConstraintType::MustNot; // now only MustNot constraints supported
                    const NSearchQuery::TMarkup& markup = wordNode->Markup();
                    for (size_t markupId = 0; markupId != NSearchQuery::MT_LIST_SIZE; ++markupId) {
                        EMarkupType markupType = static_cast<EMarkupType>(markupId);
                        const NSearchQuery::TMarkup::TItems& items = markup.GetItems(markupType);
                        if (markupId != NSearchQuery::MT_SYNONYM) {
                            continue;
                        }
                        if (!items.empty()) {
                            const TSynonym& syn = items[0].GetDataAs<TSynonym>();
                            ConstraintParts.push_back(MakeSimpleWordsOrAttributePart(*syn.SubTree));
                            ConstraintParts.back().ConstraintType = EConstraintType::MustNot;
                            NDetail::BackdoorAccess(*ConstraintParts.back().Block).Type = NDetail::EBlockType::ExactOrdered;
                        }
                    }
                }
            }
        }
    }

    void TRichNodeUnpackHelper::UnpackSynonyms(const TRichRequestNode& node, size_t begin, size_t end, bool isQuotedMultitoken)
    {
        // Cerr << "-D- Unpack synonyms " << begin << " " << end << Endl;

        Y_ASSERT(begin <= end);
        Y_ASSERT(end <= Indexes.size());

        if (begin >= end) {
            return;
        }

        const NSearchQuery::TMarkup& markup = node.Markup();

        for (size_t markupId = 0; markupId != NSearchQuery::MT_LIST_SIZE; ++markupId) {
            EMarkupType markupType = static_cast<EMarkupType>(markupId);
            const NSearchQuery::TMarkup::TItems& items = markup.GetItems(markupType);

            if (markupId != NSearchQuery::MT_SYNONYM) {
                continue;
            }

            for (size_t i = 0; i != items.size(); ++i) {
                NSearchQuery::TRange range = items[i].Range;
                const TSynonym& syn = items[i].GetDataAs<TSynonym>();

                if (!syn.SubTree) {
                    continue;
                }

                const TRichRequestNode& markupNode = *syn.SubTree;

                const ui64 typeMask = syn.GetType();

                if (typeMask & ~Opts.AllowedSynonymTypes) {
                    continue;
                }
                if (MarkupParts.size() >= Opts.MaxSynonymsPerRequest) {
                    break;
                }

                const double relev = syn.GetRelev();
                const EFormClass bestFormClass = syn.GetBestFormClass();

                size_t partFrom = GetPartIndexLowerBound(begin, end, range.Beg);
                size_t partTo = GetPartIndexLowerBound(begin, end, range.End + 1);

                // Cerr << "-D- Found markup " << markupId << " " << range.Beg << " " << range.End << " " << partFrom  << " " << partTo << Endl;

                if (partFrom >= partTo) {
                    continue;
                }

                Y_ASSERT(Indexes[partFrom] >= range.Beg);
                Y_ASSERT(Indexes[partTo - 1] <= range.End);

                TRichNodeUnpackHelper::TPart part;
                try {
                    part = MakeSimplePhrasePart(markupNode, markupType, partFrom, partTo, typeMask, relev);
                } catch (TBadPhraseError&) {
                    continue;
                }
                if (part.Block->GetNumWords() > 2 || isQuotedMultitoken) {
                    NDetail::BackdoorAccess(*part.Block).Type = NDetail::EBlockType::ExactOrdered;
                }
                MarkupParts.push_back(std::move(part));
                TrIteratorMarkupPartsBestFormClasses.push_back(bestFormClass);
            }
        }
    }

    void TRichNodeUnpackHelper::UnpackWares(const TRichRequestNode& node, size_t begin, size_t end)
    {
        Y_ASSERT(begin <= end);
        Y_ASSERT(end <= Indexes.size());

        if (begin >= end) {
            return;
        }

        const float MIN_WARES_WEIGHT = 0.7; // ATTENTION!!! to be manually picked later
        const float MAX_WARES_ONE = 0.3;    // ATTENTION!!! to be manually picked later

        const NSearchQuery::TMarkup& markup = node.Markup();
        const NSearchQuery::TMarkup::TItems& items = markup.GetItems(MT_ONTO);

        for (size_t i = 0; i != items.size(); ++i) {
            NSearchQuery::TRange range = items[i].Range;
            const TOntoTypeMarkup& onto = items[i].GetDataAs<TOntoTypeMarkup>();
            const double weight = onto.Weight;
            const double one = onto.One;

            if (weight < MIN_WARES_WEIGHT || one > MAX_WARES_ONE) {
                continue;
            }

            size_t partFrom = GetPartIndexLowerBound(begin, end, range.Beg);
            size_t partTo = GetPartIndexLowerBound(begin, end, range.End + 1);
            Y_ASSERT(partTo <= end);
            Y_ASSERT(partFrom <= partTo);

            if (partFrom >= partTo) {
                // Cerr << "-D- Skip markup, there is no word that it can be attached to" << Endl;
                continue;
            }

            Y_ASSERT(Indexes[partFrom] >= range.Beg);
            Y_ASSERT(Indexes[partTo - 1] <= range.End);

            for (size_t i2 = partFrom; i2 + 1 < partTo; ++i2) {
                Y_ASSERT (i2 < MainParts.size());
                MainParts[i2].IsWaresObjFragment = true;
            }
        }
    }

    TRichNodeUnpackHelper::TPart TRichNodeUnpackHelper::MakeSimpleWordsOrAttributePart(const TRichRequestNode& node) const
    {
        // Cerr << "-D- Make simple words or attribute node" << Endl;

        TPart wordPart(new TBlock(node));
        if (!IsAttribute(node) && node.Necessity == TNodeNecessity::nMUST && node.Children.empty()) {
            wordPart.ConstraintType = EConstraintType::Must;
        }
        wordPart.RevFreq = NLingBoost::IsValidRevFreq(node.ReverseFreq) ? node.ReverseFreq : NLingBoost::InvalidRevFreq;
        if (!LangMask.Empty()) {
            for (auto word : wordPart.Block->Words()) {
                BackdoorAccess(word).LangMask = LangMask;
            }
        }
        return wordPart;
    }

    TRichNodeUnpackHelper::TPart TRichNodeUnpackHelper::MakeSimplePhrasePart(const TRichRequestNode& node,
        EMarkupType markupType, size_t partsBegin, size_t partsEnd, size_t markupMask, double markupWeight) const
    {
        TRichNodeUnpackHelper helper(Opts);
        helper.Unpack(node);

        const TParts& parts = helper.GetMainParts();

        if (parts.size() == 0) {
            ythrow TBadPhraseError() << "empty phrase";
        }

        if (parts.size() == 1) {
            TPart phrasePart(parts[0].Block, markupType, partsBegin, partsEnd - 1);
            phrasePart.RevFreq = parts[0].RevFreq;
            phrasePart.MarkupMask = markupMask;
            phrasePart.MarkupWeight = markupWeight;

            return phrasePart;
        }

        TBlockPtr block = new TBlock;
        auto& blockContents = BackdoorAccess(*block);

        // for frequency calculation, treat AND(w1,...,wn) as the least frequent word
        i64 maxChildRevFreq = NLingBoost::InvalidRevFreq;

        size_t distance = 0;
        blockContents.Words.reserve(parts.size());
        for (size_t i = 0; i != parts.size(); ++i) {
            if (!parts[i].Block->IsWordBlock()) {
                continue;
            }
            blockContents.Words.emplace_back();
            ::DoSwap(blockContents.Words.back(), BackdoorAccess(parts[i].Block->Word()));
            maxChildRevFreq = Max<i64>(maxChildRevFreq, parts[i].RevFreq);

            if (i + 1 < parts.size()) {
                if (parts[i].IsMultitokenFragment) { // Rigid multitoken constraint
                    distance = Max<size_t>(distance, 1);
                } else {
                    const TProximity& prox = parts[i].ProxAfter;
                    if (BREAK_LEVEL == prox.Level) {
                        distance = Max<size_t>(distance, abs(prox.Beg));
                        distance = Max<size_t>(distance, abs(prox.End));
                    } else {
                        distance = 64;
                    }
                }
            }
        }
        blockContents.Distance = distance;

        TPart phrasePart(block, markupType, partsBegin, partsEnd - 1);
        if (NLingBoost::IsValidRevFreq(node.ReverseFreq)) {
            phrasePart.RevFreq = node.ReverseFreq;
        } else {
            phrasePart.RevFreq = maxChildRevFreq;
        }
        phrasePart.MarkupMask = markupMask;
        phrasePart.MarkupWeight = markupWeight;

        return phrasePart;
    }
} // NDetail
} // NReqBundle

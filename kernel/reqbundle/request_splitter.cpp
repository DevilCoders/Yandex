#include "request_splitter.h"
#include "block_pure.h"

#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>
#include <ysite/yandex/pure/pure_container.h>
#include <kernel/lingboost/constants.h>
#include <kernel/qtree/richrequest/builder/richtreebuilderaux.h>
#include <kernel/qtree/request/request.h>

using namespace NSearchQuery;

namespace NReqBundle {
    TRequestSplitter::TRequestSplitter(TSequenceAcc seq,
        TReqBundleAcc bundle,
        const TOptions& opts)
        : Options(opts)
        , UnpackHelper(opts.UnpackOptions)
        , Seq(seq)
    {
        if (bundle.IsValid()) {
            Bundle.Reset(new NDetail::THashedReqBundleAdapter(bundle));
        } else {
            Y_ASSERT(!Options.HashRequests);
        }

        if (Options.SerializeBlocks) {
            SetBlocksSerializer(Options.SerializerOptions);
        }
    }

    void TRequestSplitter::SetBlocksSerializer(const TSerializeOptions& options)
    {
        Ser.Reset(new TReqBundleSerializer(options));
    }

    void TRequestSplitter::SetPure(const TPureContainer& pure)
    {
        Pure = &pure;
    }

    size_t TRequestSplitter::CheckedAddBlock(const TBlockPtr& block)
    {
        if (Pure) {
            LoadRevFreqsFromPure(*block, *Pure);
        }

        if (Ser) {
            THolder<TBinaryBlock> binary = MakeHolder<TBinaryBlock>();
            Ser->MakeBinary(*block, *binary);
            return Seq.AddElem(binary.Release());
        } else {
            return Seq.AddElem(block);
        }
    }

    TRequestPtr TRequestSplitter::SplitRequest(const TStringBuf& text,
        const TLangMask& mask,
        const TVector<std::pair<TFacetId, float>>& facets)
    {
        const TUtf16String wText = UTF8ToWide(text);
        return SplitRequest(wText, mask, facets);
    }

    TRequestPtr TRequestSplitter::SplitRequest(const TUtf16String& wText,
        const TLangMask& mask,
        const TVector<std::pair<TFacetId, float>>& facets)
    {
        TCreateTreeOptions opts;
        opts.MakeSynonymsForMultitokens = false;

        try {
            tRequest::TNodePtr root = CreateBinaryTree<TReqAttrList>(wText, nullptr, 0).first;
            TRichNodePtr node = NRichTreeBuilder::CreateTree(*root);
            NRichTreeBuilder::PostProcessTree(node, opts);
            return SplitRequest(*node, mask, facets);
        }
        catch (...) {
            return nullptr;
        }
    }

    TRequestPtr TRequestSplitter::SplitRequestForTrIterator(
        const TRichRequestNode& node,
        const TLangMask& mask,
        const TVector<std::pair<TFacetId, float>>& facets,
        bool generateTopAndArgs)
    {
        return SplitRequestImpl(node, mask, facets, true, generateTopAndArgs);
    }

    TRequestPtr TRequestSplitter::SplitRequest(
        const TRichRequestNode& node,
        const TLangMask& mask,
        const TVector<std::pair<TFacetId, float>>& facets)
    {
        return SplitRequestImpl(node, mask, facets, false);
    }

    TRequestPtr TRequestSplitter::SplitRequestSynonyms(
        const TRichRequestNode& node,
        const TLangMask& mask,
        const TVector<std::pair<TFacetId, float>>& facets)
    {
        UnpackHelper.Unpack(node, mask);
        const NDetail::TRichNodeUnpackHelper::TParts& mainParts = UnpackHelper.GetMainParts();
        const NDetail::TRichNodeUnpackHelper::TParts& markupParts = UnpackHelper.GetMarkupParts();

        if (mainParts.empty()) {
            return nullptr;
        }

        size_t wordsCount = 0;
        TVector<size_t> mainPartWordsOffset(mainParts.size());
        for (size_t i : xrange(mainParts.size())) {
            mainPartWordsOffset[i] = wordsCount;
            const size_t numWords = mainParts[i].Block->GetNumWords();
            Y_ASSERT(numWords > 0);
            wordsCount += numWords;
        }

        Y_ASSERT(wordsCount > 0);
        if (wordsCount == 0) {
            return nullptr;
        }

        TRequestPtr request = new TRequest;
        NDetail::InitRequestData(NDetail::BackdoorAccess(*request), wordsCount);

        for (size_t i : xrange(mainParts.size())) {
            const size_t numWords = mainParts[i].Block->GetNumWords();
            if (numWords == 0) {
                continue;
            }

            if (Options.SaveWordTokens) {
                TConstBlockAcc block = *mainParts[i].Block;

                const size_t wordOffset = mainPartWordsOffset[i];

                for (size_t j : xrange(numWords)) {
                    if (wordOffset + j < wordsCount) {
                        NDetail::BackdoorAccess(*request).Words[wordOffset + j].Token = block.GetWord(j).GetText();
                    } else {
                    Y_ASSERT(false);
                    }
                }
            }
        }

        for (size_t i : xrange(markupParts.size())) {
            Y_ASSERT(markupParts[i].Type == NDetail::TRichNodeUnpackHelper::PtMarkup);
            Y_ASSERT(markupParts[i].MarkupType == NSearchQuery::MT_SYNONYM);

            const size_t blockIndex = CheckedAddBlock(markupParts[i].Block);
            const size_t partBegin = markupParts[i].PartBegin;
            const size_t partEnd = markupParts[i].PartEnd;
            Y_ASSERT(partBegin <= partEnd);
            Y_ASSERT(partEnd < mainParts.size());
            const size_t firstWord = mainPartWordsOffset[partBegin];
            const size_t lastWord = (partEnd + 1 < mainParts.size() ? mainPartWordsOffset[partEnd + 1] : wordsCount);
            Y_ASSERT(firstWord < lastWord);
            auto match = AddMatch(*request, NLingBoost::TMatch::Synonym,
                blockIndex, firstWord,
                lastWord - 1);

            match.SetSynonymMask(markupParts[i].MarkupMask);
            match.SetWeight(markupParts[i].MarkupWeight); // Not used and greater that 1.0
            match.SetRevFreq(markupParts[i].RevFreq);
        }

        for (auto& idAndValue : facets) {
            request->Facets().Set(idAndValue.first, idAndValue.second);
        }

        TRequestTrCompatibilityInfo trCompatibilityInfo;
        trCompatibilityInfo.MainPartsWordMasks = UnpackHelper.GetTrIteratorMainPartsWordMasks();
        trCompatibilityInfo.MarkupPartsBestFormClasses = UnpackHelper.GetTrIteratorMarkupPartsBestFormClasses();
        trCompatibilityInfo.WordCount = UnpackHelper.GetTrIteratorWordCount();
        request->SetTrCompatibilityInfo(trCompatibilityInfo);

        if (Bundle) {
            if (Options.HashRequests) {
                return Bundle->AddRequest(request);
            } else {
                Bundle->UnhashedReqBundle().AddRequest(request);
                return request;
            }
        } else {
            return request;
        }
    }

    TRequestPtr TRequestSplitter::CreateTelFullRequest(TReqBundle bundle) {
        static const TStringBuf TelFullAttrName = "tel_full";

        TRequestPtr request = new TRequest;
        bool attrFound = false;
        NSer::TDeserializer deserializer;
        auto seq = bundle.Sequence();
        seq.PrepareAllBlocks(deserializer);
        for (size_t i = 0; i < seq.GetNumElems(); ++i) {
            if (!seq.HasBlock(i)) {
                continue;
            }
            const auto block = seq.GetBlock(i);
            for (size_t j = 0; j < block.GetNumWords(); ++j) {
                const TString& word = block.GetWord(j).GetText();
                if (!word.StartsWith(TelFullAttrName)) {
                    continue;
                }
                attrFound = true;
                // Always using 0 WordIndex because tel_full attribute can only parse 1 number.
                NDetail::BackdoorAccess(*request).Words.resize(1);
                AddMatch(*request, NLingBoost::TMatch::OriginalWord, i, 0, 0);
            }
        }
        if (attrFound) {
            request->Facets().Set(MakeFacetId(NLingBoost::TExpansion::TelFullAttribute), 1.0);
            return request;
        }
        return nullptr;
    }

    TRequestPtr TRequestSplitter::SplitRequestImpl(
        const TRichRequestNode& node,
        const TLangMask& mask,
        const TVector<std::pair<TFacetId, float>>& facets,
        bool setTrCompatibilityInfo,
        bool generateTopAndArgs)
    {
        UnpackHelper.Unpack(node, mask);

        const NDetail::TRichNodeUnpackHelper::TParts& mainParts = UnpackHelper.GetMainParts();
        const NDetail::TRichNodeUnpackHelper::TParts& markupParts = UnpackHelper.GetMarkupParts();
        const NDetail::TRichNodeUnpackHelper::TParts& constraintParts = UnpackHelper.GetConstraintParts();
        const TVector<NDetail::TRichNodeUnpackHelper::TQuotedConstraintInfo>& quotedConstraintInfos = UnpackHelper.GetQuotedConstraintInfos();

        if (mainParts.empty()) {
            return nullptr;
        }

        size_t wordsCount = 0;
        TVector<size_t> mainPartWordsOffset(mainParts.size());
        for (size_t i : xrange(mainParts.size())) {
            mainPartWordsOffset[i] = wordsCount;
            const size_t numWords = mainParts[i].Block->GetNumWords();
            Y_ASSERT(numWords > 0);
            wordsCount += numWords;
        }

        Y_ASSERT(wordsCount > 0);
        if (wordsCount == 0) {
            return nullptr;
        }

        TRequestPtr request = new TRequest;
        NDetail::InitRequestData(NDetail::BackdoorAccess(*request), wordsCount);

        for (size_t i : xrange(mainParts.size())) {
            size_t blockIndex = CheckedAddBlock(mainParts[i].Block);

            const size_t numWords = mainParts[i].Block->GetNumWords();
            if (numWords == 0) {
                continue;
            }

            auto match = AddMatch(*request, NLingBoost::TMatch::OriginalWord, blockIndex, mainPartWordsOffset[i], mainPartWordsOffset[i] + numWords - 1);

            match.SetRevFreq(mainParts[i].RevFreq);

            if (mainParts[i].ConstraintType == EConstraintType::Must && Bundle) {
                TConstraintPtr constraint = new TConstraint();
                NDetail::BackdoorAccess(*constraint).Type = mainParts[i].ConstraintType;
                NDetail::BackdoorAccess(*constraint).BlockIndices.push_back(blockIndex);
                Bundle->AddConstraint(constraint);
            }

            if (Options.SaveWordTokens) {
                TConstBlockAcc block = *mainParts[i].Block;

                const size_t wordOffset = mainPartWordsOffset[i];

                for (size_t j : xrange(numWords)) {
                    if (wordOffset + j < wordsCount) {
                        NDetail::BackdoorAccess(*request).Words[wordOffset + j].Token = block.GetWord(j).GetText();
                    } else {
                    Y_ASSERT(false);
                    }
                }
            }
        }

        InitRequestProxes(mainParts, mainPartWordsOffset, *request);

        for (size_t i : xrange(markupParts.size())) {
            Y_ASSERT(markupParts[i].Type == NDetail::TRichNodeUnpackHelper::PtMarkup);
            Y_ASSERT(markupParts[i].MarkupType == NSearchQuery::MT_SYNONYM);

            const size_t blockIndex = CheckedAddBlock(markupParts[i].Block);
            const size_t partBegin = markupParts[i].PartBegin;
            const size_t partEnd = markupParts[i].PartEnd;
            Y_ASSERT(partBegin <= partEnd);
            Y_ASSERT(partEnd < mainParts.size());
            const size_t firstWord = mainPartWordsOffset[partBegin];
            const size_t lastWord = (partEnd + 1 < mainParts.size() ? mainPartWordsOffset[partEnd + 1] : wordsCount);
            Y_ASSERT(firstWord < lastWord);
            auto match = AddMatch(*request, NLingBoost::TMatch::Synonym,
                blockIndex, firstWord,
                lastWord - 1);

            match.SetSynonymMask(markupParts[i].MarkupMask);
            match.SetWeight(markupParts[i].MarkupWeight); // Not used and greater that 1.0
            match.SetRevFreq(markupParts[i].RevFreq);
        }

        TVector<size_t> constraintPartsBlockIndices(constraintParts.size());
        if (!constraintParts.empty()) {
            if (Bundle) {
                for (size_t i : xrange(constraintParts.size())) {
                    size_t blockIndex = CheckedAddBlock(constraintParts[i].Block);

                    constraintPartsBlockIndices[i] = blockIndex;

                    if (constraintParts[i].ConstraintType != NLingBoost::TConstraint::Quoted) {
                        TConstraintPtr constraint = new TConstraint();
                        NDetail::BackdoorAccess(*constraint).Type = constraintParts[i].ConstraintType;
                        NDetail::BackdoorAccess(*constraint).BlockIndices.push_back(blockIndex);

                        Bundle->AddConstraint(constraint);
                    }
                }
            } else {
                Y_ASSERT(false);
            }
        }

        TVector<size_t> additionalBlockWordMask;
        if (!quotedConstraintInfos.empty()) {
            if (Bundle) {
                for (const NDetail::TRichNodeUnpackHelper::TQuotedConstraintInfo& quotedConstraintInfo : quotedConstraintInfos) {
                    TConstraintPtr constraint = new TConstraint();
                    NDetail::BackdoorAccess(*constraint).Type = NLingBoost::TConstraint::Quoted;

                    for (const NDetail::TRichNodeUnpackHelper::TQuotedConstraintPartRange& quotedPartRange : quotedConstraintInfo) {
                        for (size_t pos = quotedPartRange.Begin; pos < quotedPartRange.End; pos++) {
                            if (pos >= constraintPartsBlockIndices.size()) {
                                Y_ASSERT(false);
                                break;
                            }
                            NDetail::BackdoorAccess(*constraint).BlockIndices.push_back(constraintPartsBlockIndices[pos]);
                        }
                    }

                    Bundle->AddConstraint(constraint);
                }
            } else {
                Y_ASSERT(false);
            }
        }

        if (setTrCompatibilityInfo) {
            TRequestTrCompatibilityInfo trCompatibilityInfo;
            trCompatibilityInfo.MainPartsWordMasks = UnpackHelper.GetTrIteratorMainPartsWordMasks();
            trCompatibilityInfo.MarkupPartsBestFormClasses = UnpackHelper.GetTrIteratorMarkupPartsBestFormClasses();
            trCompatibilityInfo.WordCount = UnpackHelper.GetTrIteratorWordCount();
            if (generateTopAndArgs) {
                if (TTopAndArgsForWeb topAndArgsForWeb; GenerateTopAndArgsForWeb(node, &topAndArgsForWeb) == ELoadResult::LR_OK) {
                    trCompatibilityInfo.TopAndArgsForWeb = std::move(topAndArgsForWeb);
                }
            }

            request->SetTrCompatibilityInfo(trCompatibilityInfo);
        }

        for (auto& idAndValue : facets) {
            request->Facets().Set(idAndValue.first, idAndValue.second);
        }

        if (Bundle) {
            if (Options.HashRequests) {
                return Bundle->AddRequest(request);
            } else {
                Bundle->UnhashedReqBundle().AddRequest(request);
                return request;
            }
        } else {
            return request;
        }
    }

    TMatchAcc TRequestSplitter::AddSynonym(TRequestAcc request,
        const TStringBuf& text,
        size_t fromIndex,
        size_t toIndex,
        const TLangMask& mask)
    {
        TUtf16String wText = UTF8ToWide(text);
        TCreateTreeOptions opts;
        opts.MakeSynonymsForMultitokens = false;

        TRichNodePtr node;
        try {
            tRequest::TNodePtr root = CreateBinaryTree<TReqAttrList>(wText, nullptr, 0).first;
            node = NRichTreeBuilder::CreateTree(*root);
            NRichTreeBuilder::PostProcessTree(node, opts);
        }
        catch (...) {
            return TMatchAcc();
        }

        return AddSynonym(request, *node, fromIndex, toIndex, mask);
    }

    TMatchAcc TRequestSplitter::AddSynonym(TRequestAcc request,
        const TRichRequestNode& node,
        size_t fromIndex,
        size_t toIndex,
        const TLangMask& mask)
    {
        THolder<TBlock> block = MakeHolder<TBlock>(node);

        if (!mask.Empty()) {
            for (auto word : block->Words()) {
                word.SetLangMask(mask);
            }
        }

        size_t blockIndex = CheckedAddBlock(block.Release());
        return AddMatch(
            request, NLingBoost::TMatch::Synonym,
            blockIndex, fromIndex, toIndex);
    }

    TMatchAcc TRequestSplitter::AddMatch(TRequestAcc request, EMatchType type,
        size_t blockIndex, size_t fromIndex, size_t toIndex)
    {
        Y_ASSERT(toIndex < request.GetNumWords());
        Y_ASSERT(fromIndex <= toIndex);

        auto& matches = NDetail::BackdoorAccess(request).Matches;
        matches.emplace_back();
        auto& matchData = matches.back();

        matchData.Type = type;
        matchData.BlockIndex = blockIndex;
        matchData.WordIndexFirst = fromIndex;
        matchData.WordIndexLast = toIndex;
        return TMatchAcc(matchData);
    }

    void TRequestSplitter::InitRequestProxes(
        const NDetail::TRichNodeUnpackHelper::TParts& mainParts,
        const TVector<size_t>& mainPartWordsOffset,
        TRequestAcc request)
    {
        for (size_t i = 0; i + 1 < mainParts.size(); ++i) {
            if (mainParts[i].IsMultitokenFragment) {
                Y_ASSERT(mainPartWordsOffset[i + 1] > 0);
                request.ProxAfter(mainPartWordsOffset[i + 1] - 1).SetMultitoken(true);
            }

            if (mainParts[i].IsMultitokenFragment
                || mainParts[i].IsWaresObjFragment
                || (mainParts[i].ProxAfter.Beg == 1 && mainParts[i].ProxAfter.End == 1))
            {
                Y_ASSERT(mainPartWordsOffset[i + 1] > 0);
                request.ProxAfter(mainPartWordsOffset[i + 1] - 1).SetCohesion(1.0f);
            }
        }
    }
} // NReqBundle

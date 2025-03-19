#include "hits_map.h"
#include "expansions_resolver.h"

#include <kernel/reqbundle/reqbundle.h>
#include <kernel/reqbundle/serializer.h>
#include <kernel/text_machine/util/hits_serializer.h>
#include <kernel/text_machine/proto/text_machine.pb.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/stream/format.h>
#include <util/generic/set.h>
#include <util/generic/xrange.h>


using namespace NTextMachine;

namespace NTextMachine {
    void THitsMap::AppendHitForDoc(
        THitsMap::TDoc& doc,
        const NTextMachine::TBlockHit& blockHit,
        TMaybe<NLingBoost::EExpansionType> expansionType,
        TExpansionsResolver& resolver)
    {
        TExpansionsResolver::TIndexes requestIndexes = resolver.GetExpansionsByBlockId(blockHit.BlockId);

        if (expansionType.Defined()) {
            TExpansionsResolver::TIndexes corrRequestIndexes;

            bool isAccepted = false;
            for (size_t requestIndex : requestIndexes) {
                NReqBundle::TConstRequestAcc request = doc.Bundle.GetRequest(requestIndex);
                for (NReqBundle::TConstFacetEntryAcc entry : request.GetFacets().GetEntries()) {
                    if (entry.GetExpansion() == expansionType.GetRef()) {
                        corrRequestIndexes.insert(requestIndex);
                        isAccepted = true;
                        break;
                    }
                }
            }

            if (!isAccepted) {
                return;
            }

            requestIndexes.swap(corrRequestIndexes);
        }

        doc.Storage.Hits.push_back(blockHit);
        NTextMachine::TBlockHit* savedHitPtr = &doc.Storage.Hits.back();

        NTextMachine::TPosition& pos = savedHitPtr->Position;
        const NTextMachine::TAnnotation& ann = pos.AnnotationRef();

        ui32 resolvedBreakNumber = ResolveBreakNumber(ann.StreamRef().Type, ann.BreakNumber);
        if (pos.LeftWordPos > pos.RightWordPos) {
            Cerr << "-W- Invalid position in stream " << ann.Stream->Type
                << " {BreakId: " << ann.BreakNumber
                << ", resolvedBreakNumber: " << resolvedBreakNumber
                << ", Left: " << pos.LeftWordPos
                << ", Right: " << pos.RightWordPos
                << ", Length: " << ann.Length << "}" << Endl;

            std::swap(pos.LeftWordPos, pos.RightWordPos);
        }

        THitsMap::TSentence& sent = doc.Sentences[{resolvedBreakNumber, ann.StreamRef().Type}];
        sent.Value = ann.Value;

        if (ann.Length != NTextMachine::TAbsentValue::BreakWordCount) {
            if (sent.Words.size() < ann.Length) {
                sent.Words.resize(ann.Length);
            }
        }

        if (sent.Words.size() <= pos.RightWordPos) {
            sent.Words.resize(pos.RightWordPos + 1);
        }

        for (size_t wordPos : xrange<size_t>(pos.LeftWordPos, pos.RightWordPos + 1)) {
            THitsMap::TWord& word = sent.Words[wordPos];

            for (size_t requestIndex: requestIndexes) {
                NReqBundle::TConstRequestAcc request = doc.Bundle.GetRequest(requestIndex);

                for (NReqBundle::TConstFacetEntryAcc entry : request.GetFacets().GetEntries()) {
                    if (expansionType.Defined() && entry.GetExpansion() != expansionType) {
                        continue;
                    }

                    for (NReqBundle::TConstMatchAcc match : request.GetMatches()) {
                        if (match.GetBlockIndex() == blockHit.BlockId) {
                            THitsMap::THit& hit = word.Hits.emplace_back();
                            hit.BlockHit = savedHitPtr;
                            hit.Type = match.GetType() == EMatchType::Synonym
                                ? EHitType::Synonym
                                : (blockHit.Precision == EMatchPrecisionType::Exact ? EHitType::Exact : EHitType::Lemma);

                            hit.DualType = entry.GetExpansion();
                            hit.DualValue = entry.GetValue();
                            hit.DualIdx = requestIndex;
                            hit.DualWordBegin = match.GetWordIndexFirst();
                            hit.DualWordEnd = match.GetWordIndexLast();
                            hit.DualLength = request.GetNumWords();
                        }
                    }
                }
            }
        }
    }

    void THitsMap::AppendHitForRequest(
        THitsMap::TDoc& doc,
        const NTextMachine::TBlockHit& blockHit,
        TMaybe<NLingBoost::EExpansionType> expansionType,
        TExpansionsResolver& resolver)
    {
        doc.Storage.Hits.push_back(blockHit);
        const NTextMachine::TBlockHit* savedHitPtr = &doc.Storage.Hits.back();
        TExpansionsResolver::TIndexes requestIndexes = resolver.GetExpansionsByBlockId(blockHit.BlockId);

        for (size_t requestIndex : requestIndexes) {
            NReqBundle::TConstRequestAcc request = doc.Bundle.GetRequest(requestIndex);

            for (NReqBundle::TConstFacetEntryAcc entry : request.GetFacets().GetEntries()) {
                if (expansionType.Defined() && entry.GetExpansion() != expansionType) {
                    continue;
                }

                THitsMap::TSentence& sent = doc.Sentences[{static_cast<ui32>(requestIndex), entry.GetExpansion()}];
                sent.Value = entry.GetValue();

                if (sent.Words.size() < request.GetNumWords()) {
                    Y_ENSURE(sent.Words.empty());
                    sent.Words.resize(request.GetNumWords());
                }

                for (NReqBundle::TConstMatchAcc match : request.GetMatches()) {
                    if (match.GetBlockIndex() != blockHit.BlockId) {
                        continue;
                    }

                    for (size_t wordPos : match.GetWordIndexes()) {
                        THitsMap::TWord& word = sent.Words[wordPos];
                        THitsMap::THit& hit = word.Hits.emplace_back();

                        hit.BlockHit = savedHitPtr;
                        hit.Type = match.GetType() == EMatchType::Synonym
                            ? EHitType::Synonym
                            : (blockHit.Precision == EMatchPrecisionType::Exact ? EHitType::Exact : EHitType::Lemma);

                        hit.DualType = blockHit.StreamRef().Type;
                        hit.DualValue = blockHit.AnnotationRef().Value;
                        hit.DualIdx = ResolveBreakNumber(blockHit.StreamRef().Type, blockHit.AnnotationRef().BreakNumber);
                        hit.DualWordBegin = blockHit.Position.LeftWordPos;
                        hit.DualWordEnd = blockHit.Position.RightWordPos;
                        hit.DualLength = blockHit.AnnotationRef().Length;
                    }
                }
            }
        }
    }

    THolder<THitsMap::TDoc> THitsMap::CreateForDoc(
        const NTextMachineProtocol::TPbDocHits& docHits,
        const TMaybe<NLingBoost::EExpansionType>& expansionType,
        const TSet<NLingBoost::EStreamType>& streamTypes)
    {
        THolder<TDoc> doc = MakeHolder<TDoc>();

        {
            NReqBundle::NSer::TDeserializer deser;
            TString binary = docHits.HasBinaryBundle() ? docHits.GetBinaryBundle() : Base64Decode(docHits.GetQBundleBase64());
            deser.Deserialize(binary, doc->Bundle);
            doc->Bundle.Sequence().PrepareAllBlocks(deser);
        }
        TExpansionsResolver resolver{doc->Bundle};

        NTextMachine::THitsDeserializer deser{&docHits.GetHits()};
        deser.Init();

        NTextMachine::TBlockHit blockHit;
        while (deser.NextBlockHit(blockHit)) {
            if (!streamTypes.empty() && !streamTypes.contains(blockHit.Position.AnnotationRef().StreamRef().Type)) {
                continue;
            }
            AppendHitForDoc(*doc, blockHit, expansionType, resolver);
        }

        doc->Storage.AuxData = std::move(deser.GetHitsAuxData());

        return doc;
    }

    THolder<THitsMap::TDoc> THitsMap::CreateForRequest(
        const NTextMachineProtocol::TPbDocHits& docHits,
        const TMaybe<NLingBoost::EExpansionType>& expansionType,
        const TSet<NLingBoost::EStreamType>& streamTypes)
    {
        THolder<TDoc> doc = MakeHolder<TDoc>();

        {
            NReqBundle::NSer::TDeserializer deser;
            TString binary = docHits.HasBinaryBundle() ? docHits.GetBinaryBundle() : Base64Decode(docHits.GetQBundleBase64());
            deser.Deserialize(binary, doc->Bundle);
            doc->Bundle.Sequence().PrepareAllBlocks(deser);
        }
        TExpansionsResolver resolver{doc->Bundle};

        NTextMachine::THitsDeserializer deser{&docHits.GetHits()};
        deser.Init();

        NTextMachine::TBlockHit blockHit;
        while (deser.NextBlockHit(blockHit)) {
            if (!streamTypes.empty() && !streamTypes.contains(blockHit.AnnotationRef().StreamRef().Type)) {
                continue;
            }
            AppendHitForRequest(*doc, blockHit, expansionType, resolver);
        }

        doc->Storage.AuxData = std::move(deser.GetHitsAuxData());

        return doc;
    }

    TVector<TString> THitsMap::GetRequestWords(const THolder<TDoc>& doc, const size_t requestIndex) {
        NReqBundle::TConstRequestAcc request = doc->Bundle.GetRequest(requestIndex);
        TVector<TString> result(request.GetNumWords());
        NReqBundle::TConstSequenceAcc seq = doc->Bundle.Sequence();
        for (auto match : request.GetMatches()) {
            if (match.GetType() == TMatch::OriginalWord) {
                for (size_t wordIndex : match.GetWordIndexes()) {
                    auto elem = seq.GetElem(match);
                    if (elem.HasBlock() && elem.GetBlock().GetNumWords() >= 1) {
                        auto block = seq.GetElem(match).GetBlock();
                        result[wordIndex] = block.GetWord().GetText();
                    }
                }
            }
        }
        return result;
    }

} // NTextMachine

#include "hits_map.h"

#include <kernel/reqbundle/print.h>
#include <kernel/reqbundle/serializer.h>

#include <library/cpp/string_utils/base64/base64.h>

#include <util/string/builder.h>
#include <util/string/join.h>
#include <util/stream/format.h>
#include <util/generic/set.h>
#include <util/generic/xrange.h>


namespace {
    void PrintSet(IOutputStream& out, const TSet<TString>& texts) {
        if (texts.empty()) {
            out << '-';
        } else if (texts.size() == 1) {
            out << *texts.begin();
        } else {
            out << '{' << JoinSeq(",", texts) << '}';
        }
    }

    class TExpansionsResolver {
    public:
        using TIndexes = TSet<size_t>;

        TExpansionsResolver(NReqBundle::TConstReqBundleAcc bundle)
            : Bundle(bundle)
        {}

        const TIndexes& GetExpansionsByBlockId(size_t blockId) {
            if (auto ptr = ExpansionsByBlockId.FindPtr(blockId)) {
                return *ptr;
            }

            TIndexes expansions;
            for (size_t requestIndex : xrange(Bundle.GetNumRequests())) {
                const NReqBundle::TConstRequestAcc request = Bundle.GetRequest(requestIndex);
                for (const NReqBundle::TConstMatchAcc match : request.GetMatches()) {
                    if (match.GetBlockIndex() == blockId) {
                        expansions.insert(requestIndex);
                        break;
                    }
                }
            }

            return (ExpansionsByBlockId[blockId] = expansions);
        }

    private:
        TMap<size_t, TIndexes> ExpansionsByBlockId;
        NReqBundle::TConstReqBundleAcc Bundle;
    };

    class TPrinterBase
        : public THitsMap::IPrinter
    {
    public:
        void PrintSentence(
            IOutputStream& out,
            const THitsMap::TSentence& sentence) const override
        {
            bool first = true;
            for (const THitsMap::TWord& word : sentence.Words) {
                if (!first) {
                    out << ' ';
                } else {
                    first = false;
                }

                PrintWord(out, word);
            }
        }

        void PrintDoc(
            IOutputStream& out,
            const THitsMap::TDoc& doc) const override
        {
            for (const auto& entry : doc.Sentences) {
                ui32 id = entry.first;
                const THitsMap::TSentence& sentence = entry.second;

                out << LeftPad(id, 8) << '\t';
                if (sentence.Values.size() == 1) {
                    out << Prec(sentence.Values.front(), PREC_POINT_DIGITS, 6) << ' ';
                } else if (sentence.Values.size() > 1) {
                    out << Prec(sentence.Values.front(), PREC_POINT_DIGITS, 4) << ".. ";
                } else {
                    out << "       ";
                }
                PrintSentence(out, sentence);
                out << '\n';
            }
        }
    };

    class TRequestPrinterBase
        : public TPrinterBase
    {
    public:
        void PrintSentence(
            IOutputStream& out,
            const THitsMap::TSentence& sentence) const override;

        void PrintDoc(
            IOutputStream& out,
            const THitsMap::TDoc& doc) const override;
    };

    class TTokenPrinter
        : public TPrinterBase
    {
    public:
        ~TTokenPrinter() override {}

        void PrintWord(
            IOutputStream& out,
            const THitsMap::TWord& word) const override
        {
            PrintSet(out, word.Hits.empty()
                ? TSet<TString>{}
                : TSet<TString>{"X"});
        }
    };

    class TBlockIdPrinter
        : public TPrinterBase
    {
    public:
        ~TBlockIdPrinter() override {}

        void PrintWord(
            IOutputStream& out,
            const THitsMap::TWord& word) const override
        {
            TSet<TString> texts;
            for (const NTextMachine::TBlockHit* blockHit : word.Hits) {
                texts.insert(ToString(blockHit->BlockId));
            }

            PrintSet(out, texts);
        }
    };

    class TStreamPrinter
        : public TPrinterBase
    {
    public:
        ~TStreamPrinter() override {}

        TStreamPrinter(bool isBreakId)
            : IsBreakId(isBreakId)
        {}

        void PrintWord(
            IOutputStream& out,
            const THitsMap::TWord& word) const override
        {
            TSet<TString> texts;
            for (const NTextMachine::TBlockHit* blockHit : word.Hits) {
                const NTextMachine::TAnnotation& ann = blockHit->AnnotationRef();

                if (IsBreakId) {
                    texts.insert(ToString(ann.BreakNumber));
                } else if (ann.StreamIndex != 0) {
                    texts.insert(TStringBuilder{}
                        << ann.StreamRef().Type << '.' << ann.StreamIndex);
                } else {
                    texts.insert(ToString(ann.StreamRef().Type));
                }
            }

            PrintSet(out, texts);
        }

    private:
        bool IsBreakId = false;
    };

    class TWordPrinter
        : public TPrinterBase
    {
    public:
        enum class EWordTextType {
            Original,
            MatchedForm
        };

        ~TWordPrinter() override {}

        TWordPrinter(NReqBundle::TConstSequenceAcc seq, EWordTextType textType = EWordTextType::Original)
            : Seq(seq)
            , TextType(textType)
        {}

        void PrintWord(
            IOutputStream& out,
            const THitsMap::TWord& word) const override
        {
            TSet<TString> texts;
            for (const NTextMachine::TBlockHit* blockHit : word.Hits) {
                NTextMachine::TBlockId blockId = blockHit->BlockId;
                NTextMachine::TWordLemmaId lemmaId = blockHit->LemmaId;
                NTextMachine::TWordFormId formId = blockHit->FormId;
                NReqBundle::TConstBlockAcc block = Seq.GetBlock(blockId);

                TString text;
                TStringOutput out{text};
                out << NReqBundle::PrintableBlock(block);
                if (block.GetNumWords() == 1) {
                    NReqBundle::TConstWordAcc word = block.GetWord();
                    switch (TextType) {
                        case EWordTextType::Original: {
                            break;
                        }
                        case EWordTextType::MatchedForm: {
                            out << "(" << blockId << ")";
                            if (lemmaId >= word.GetNumLemmas()) {
                                out << ".(" << lemmaId << ")." << "(" << formId << ")";
                            } else {
                                NReqBundle::TConstLemmaAcc lemma = word.GetLemma(lemmaId);
                                out << "." << lemma.GetText() << "(" << lemmaId << ")";
                                out << ".(" << formId << ")";
                            }
                            break;
                        }
                    }
                    out << "[" << blockHit->Position.LeftWordPos << ":" << blockHit->Position.RightWordPos << "]";
                }

                texts.insert(text);
            }

            PrintSet(out, texts);
        }

    private:
        NReqBundle::TConstSequenceAcc Seq;
        EWordTextType TextType = EWordTextType::Original;
    };

    class TExpansionPrinter
        : public TPrinterBase
    {
    private:
        using TExpansions = TSet<std::pair<size_t, NLingBoost::EExpansionType>>;

        TExpansions GetExpansions(size_t blockId) const {
            TExpansionsResolver::TIndexes indexes = Resolver.GetExpansionsByBlockId(blockId);
            TExpansions expansions;

            for (size_t requestIndex : indexes) {
                NReqBundle::TConstRequestAcc request = Bundle.GetRequest(requestIndex);
                for (const NReqBundle::TConstFacetEntryAcc facet : request.GetFacets().GetEntries()) {
                    expansions.emplace(requestIndex, facet.GetExpansion());
                }
            }

            return expansions;
        }

        bool IsRequestId = false;
        NReqBundle::TConstReqBundleAcc Bundle;
        mutable TExpansionsResolver Resolver;

    public:
        ~TExpansionPrinter() override {}

        TExpansionPrinter(NReqBundle::TConstReqBundleAcc bundle, bool isRequestId)
            : IsRequestId(isRequestId)
            , Bundle(bundle)
            , Resolver(bundle)
        {}

        void PrintWord(
            IOutputStream& out,
            const THitsMap::TWord& word) const override
        {
            if (word.Hits.empty()) {
                out << '-';
                return;
            }

            TSet<TString> texts;
            for (const NTextMachine::TBlockHit* blockHit : word.Hits) {
                NTextMachine::TBlockId blockId = blockHit->BlockId;

                for (std::pair<size_t, NLingBoost::EExpansionType> indexAndType: GetExpansions(blockId)) {
                    if (IsRequestId) {
                        texts.insert(ToString(indexAndType.first));
                    } else {
                        texts.insert(ToString(indexAndType.second));
                    }
                }
            }

            if (texts.size() == 1) {
                out << *texts.begin();
                return;
            }

            out << '{' << JoinSeq(",", texts) << '}';
        }
    };

    void AppendHitForDoc(
        THitsMap::TDoc& doc,
        const NTextMachine::TBlockHit& blockHit,
        NReqBundle::TConstReqBundleAcc bundle,
        TMaybe<NLingBoost::EExpansionType> expansionType,
        TExpansionsResolver& resolver)
    {
        if (expansionType.Defined()) {
            bool isAccepted = false;
            TExpansionsResolver::TIndexes requestIndexes = resolver.GetExpansionsByBlockId(blockHit.BlockId);
            for (size_t requestIndex : requestIndexes) {
                NReqBundle::TConstRequestAcc request = bundle.GetRequest(requestIndex);
                for (NReqBundle::TConstFacetEntryAcc entry : request.GetFacets().GetEntries()) {
                    if (entry.GetExpansion() == expansionType.GetRef()) {
                        isAccepted = true;
                        break;
                    }
                }

                if (isAccepted) {
                    break;
                }
            }

            if (!isAccepted) {
                return;
            }
        }

        doc.Storage.Hits.push_back(blockHit);
        NTextMachine::TBlockHit* savedHitPtr = &doc.Storage.Hits.back();

        NTextMachine::TPosition& pos = savedHitPtr->Position;
        const NTextMachine::TAnnotation& ann = pos.AnnotationRef();

        if (pos.LeftWordPos > pos.RightWordPos) {
            Cerr << "-W- Invalid position in stream " << ann.Stream->Type
                << " {BreakId: " << ann.BreakNumber
                << ", Left: " << pos.LeftWordPos
                << ", Right: " << pos.RightWordPos
                << ", Length: " << ann.Length << "}" << Endl;

            std::swap(pos.LeftWordPos, pos.RightWordPos);
        }

        THitsMap::TSentence& sent = doc.Sentences[ann.BreakNumber];

        if (Find(sent.Values.begin(), sent.Values.end(), ann.Value) == sent.Values.end()) {
            sent.Values.push_back(ann.Value);
        }

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
            word.Hits.push_back(savedHitPtr);
        }
    }

    void AppendHitForRequest(
        THitsMap::TDoc& doc,
        const NTextMachine::TBlockHit& blockHit,
        NReqBundle::TConstReqBundleAcc bundle,
        TMaybe<NLingBoost::EExpansionType> expansionType,
        TExpansionsResolver& resolver)
    {
        doc.Storage.Hits.push_back(blockHit);
        const NTextMachine::TBlockHit* savedHitPtr = &doc.Storage.Hits.back();
        TExpansionsResolver::TIndexes requestIndexes = resolver.GetExpansionsByBlockId(blockHit.BlockId);

        for (size_t requestIndex : requestIndexes) {
            NReqBundle::TConstRequestAcc request = bundle.GetRequest(requestIndex);

            for (NReqBundle::TConstFacetEntryAcc entry : request.GetFacets().GetEntries()) {
                if (expansionType.Defined() && entry.GetExpansion() != expansionType) {
                    continue;
                }

                THitsMap::TSentence& sent = doc.Sentences[requestIndex];

                if (Find(sent.Values.begin(), sent.Values.end(), entry.GetValue()) == sent.Values.end()) {
                    sent.Values.push_back(entry.GetValue());
                }

                if (sent.Words.size() <= request.GetNumWords()) {
                    sent.Words.resize(request.GetNumWords());
                }

                for (NReqBundle::TConstMatchAcc match : request.GetMatches()) {
                    if (match.GetBlockIndex() != blockHit.BlockId) {
                        continue;
                    }

                    for (size_t wordPos : match.GetWordIndexes()) {
                        THitsMap::TWord& word = sent.Words[wordPos];
                        word.Hits.push_back(savedHitPtr);
                    }
                }
            }
        }
    }
} // namespace

THolder<THitsMap::TDoc> THitsMap::CreateForDoc(
    const NTextMachineProtocol::TPbDocHits& docHits,
    const TMaybe<NLingBoost::EExpansionType>& expansionType,
    const TMaybe<NLingBoost::EStreamType>& streamType)
{
    THolder<TDoc> doc = MakeHolder<TDoc>();

    TReqBundle bundle;
    {
        NReqBundle::NSer::TDeserializer deser;
        TString binary = docHits.HasBinaryBundle() ? docHits.GetBinaryBundle() : Base64Decode(docHits.GetQBundleBase64());
        deser.Deserialize(binary, bundle);
        bundle.Sequence().PrepareAllBlocks(deser);
    }
    TExpansionsResolver resolver{bundle};

    NTextMachine::THitsDeserializer deser{&docHits.GetHits()};
    deser.Init();

    NTextMachine::TBlockHit blockHit;
    while (deser.NextBlockHit(blockHit)) {
        if (streamType.Defined() && streamType != blockHit.Position.AnnotationRef().StreamRef().Type) {
            continue;
        }
        AppendHitForDoc(*doc, blockHit, bundle, expansionType, resolver);
    }

    doc->Storage.AuxData = std::move(deser.GetHitsAuxData());

    return doc;
}

THolder<THitsMap::TDoc> THitsMap::CreateForRequest(
    const NTextMachineProtocol::TPbDocHits& docHits,
    const TMaybe<NLingBoost::EExpansionType>& expansionType,
    const TMaybe<NLingBoost::EStreamType>& streamType)
{
    THolder<TDoc> doc = MakeHolder<TDoc>();

    TReqBundle bundle;
    {
        NReqBundle::NSer::TDeserializer deser;
        TString binary = docHits.HasBinaryBundle() ? docHits.GetBinaryBundle() : Base64Decode(docHits.GetQBundleBase64());
        deser.Deserialize(binary, bundle);
        bundle.Sequence().PrepareAllBlocks(deser);
    }
    TExpansionsResolver resolver{bundle};

    NTextMachine::THitsDeserializer deser{&docHits.GetHits()};
    deser.Init();

    NTextMachine::TBlockHit blockHit;
    while (deser.NextBlockHit(blockHit)) {
        if (streamType.Defined() && streamType != blockHit.AnnotationRef().StreamRef().Type) {
            continue;
        }
        AppendHitForRequest(*doc, blockHit, bundle, expansionType, resolver);
    }

    doc->Storage.AuxData = std::move(deser.GetHitsAuxData());

    return doc;
}

THolder<THitsMap::IPrinter> THitsMap::CreateTokenPrinter() {
    return MakeHolder<TTokenPrinter>();
}

THolder<THitsMap::IPrinter> THitsMap::CreateBlockIdPrinter() {
    return MakeHolder<TBlockIdPrinter>();
}

THolder<THitsMap::IPrinter> THitsMap::CreateStreamPrinter() {
    return MakeHolder<TStreamPrinter>(false);
}

THolder<THitsMap::IPrinter> THitsMap::CreateBreakIdPrinter() {
    return MakeHolder<TStreamPrinter>(true);
}

THolder<THitsMap::IPrinter> THitsMap::CreateWordPrinter(NReqBundle::TConstSequenceAcc seq) {
    return MakeHolder<TWordPrinter>(seq);
}

THolder<THitsMap::IPrinter> THitsMap::CreateFormPrinter(NReqBundle::TConstSequenceAcc seq) {
    return MakeHolder<TWordPrinter>(seq, TWordPrinter::EWordTextType::MatchedForm);
}

THolder<THitsMap::IPrinter> THitsMap::CreateExpansionPrinter(NReqBundle::TConstReqBundleAcc bundle) {
    return MakeHolder<TExpansionPrinter>(bundle, false);
}

THolder<THitsMap::IPrinter> THitsMap::CreateRequestIdPrinter(NReqBundle::TConstReqBundleAcc bundle) {
    return MakeHolder<TExpansionPrinter>(bundle, true);
}

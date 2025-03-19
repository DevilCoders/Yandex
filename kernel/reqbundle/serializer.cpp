#include "serializer.h"

#include "size_limits.h"

#include <kernel/factor_storage/float_utils.h>
#include <kernel/hitinfo/hitinfo.h>
#include <kernel/qtree/richrequest/richnode.h>

#include <library/cpp/langmask/serialization/langmask.h>
#include <library/cpp/streams/lz/lz.h>

#include <util/digest/city.h>
#include <util/stream/format.h>
#include <util/stream/buffer.h>

namespace {
    using namespace NReqBundle;

    using TErrorGuard = NLingBoost::TErrorHandler::TGuard;

    template <typename ProtoType>
    const ProtoType& GetDefault() {
        static ProtoType defaultProto{};
        return defaultProto;
    }

    template <typename ProtoType>
    const ProtoType& GetDefault(const ProtoType&) {
        return GetDefault<ProtoType>();
    }

    struct TSequenceElemDataWithIndex {
        const NReqBundle::NDetail::TSequenceElemData* Ptr = nullptr;
        size_t Index = Max<size_t>();

        TSequenceElemDataWithIndex() = default;
        TSequenceElemDataWithIndex(
            const NReqBundle::NDetail::TSequenceElemData* ptr,
            size_t index)
            : Ptr(ptr)
            , Index(index)
        {}

        bool operator < (const TSequenceElemDataWithIndex& other) const {
            return TConstSequenceElemAcc(*Ptr) < TConstSequenceElemAcc(*other.Ptr);
        }
    };
} // namespace

namespace NReqBundle {
namespace NSer {
    void TSerializer::SerializeToProto(const NLingBoost::TRevFreq& revFreq, TRevFreqsArrayProto& proto) const
    {
        for (const auto& entry : revFreq.Values) {
            if (NLingBoost::IsValidRevFreq(entry.Value())) {
                NReqBundleProtocol::TRevFreqValue& valueProto = *proto.Add();
                if (static_cast<ui32>(entry.Key()) != GetDefault(valueProto).GetFreqType()) {
                    valueProto.SetFreqType(static_cast<ui32>(entry.Key()));
                }
                valueProto.SetRevFreq(entry.Value());
            }
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TFormData& formData, size_t prefix, NReqBundleProtocol::TBlockFormInfo& proto) const
    {
        proto.SetText(formData.Text.GetUtf8().substr(prefix));
        proto.SetExact(formData.Exact);
    }

    void TSerializer::SerializeToProto(const NDetail::TLemmaData& lemmaData, NReqBundleProtocol::TBlockLemmaInfo& proto) const
    {
        proto.SetText(lemmaData.Text.GetUtf8());
        proto.SetLanguage(lemmaData.Language);
        proto.SetBest(lemmaData.Best);
        if (lemmaData.Attribute != GetDefault(proto).GetAttribute()) {
            proto.SetAttribute(lemmaData.Attribute);
        }
        SerializeToProto(lemmaData.RevFreq, *proto.MutableRevFreqsByType());
        TVector<const NDetail::TFormData*> formPtrs;
        formPtrs.reserve(lemmaData.Forms.size());

        for (const NDetail::TFormData& formData : lemmaData.Forms) {
            formPtrs.push_back(&formData);
        }

        Sort(formPtrs.begin(), formPtrs.end(),
            [](const NDetail::TFormData* lhs, const NDetail::TFormData* rhs) {
                return lhs->Text.GetUtf8() < rhs->Text.GetUtf8();
            });

        for (size_t i = 0; i < lemmaData.Forms.size(); ++i) {
            if (i && Options.UseDeltaFormsEncoding) {
                size_t bound = Min(formPtrs[i]->Text.GetUtf8().size(), formPtrs[i - 1]->Text.GetUtf8().size());
                size_t j = 0;
                while (j < bound && formPtrs[i]->Text.GetUtf8()[j] == formPtrs[i - 1]->Text.GetUtf8()[j]) {
                    ++j;
                }
                proto.AddFormsPrefixSizes(j);
                SerializeToProto(*formPtrs[i], j, *proto.AddForms());
            } else {
                SerializeToProto(*formPtrs[i], 0, *proto.AddForms());
            }
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TWordData& wordData, NReqBundleProtocol::TBlockWordInfo& proto) const
    {
        proto.SetText(wordData.Text.GetUtf8());
        ::Serialize(*proto.MutableLanguages(), wordData.LangMask, false);
        proto.SetCaseFlags(wordData.CaseFlags);
        proto.SetNlpType(wordData.NlpType);
        proto.SetStopWord(wordData.StopWord);
        if (wordData.AnyWord != GetDefault(proto).GetAnyWord()) {
            proto.SetAnyWord(wordData.AnyWord);
        }
        SerializeToProto(wordData.RevFreq, *proto.MutableRevFreqsByType());
        SerializeToProto(wordData.RevFreqAllForms, *proto.MutableRevFreqsAllFormsByType());

        TVector<const NDetail::TLemmaData*> lemmaPtrs;
        lemmaPtrs.reserve(wordData.Lemmas.size());

        for (const NDetail::TLemmaData& lemmaData : wordData.Lemmas) {
            lemmaPtrs.push_back(&lemmaData);
        }

        StableSort(lemmaPtrs.begin(), lemmaPtrs.end(),
            [](const NDetail::TLemmaData* x, const NDetail::TLemmaData* y) {
                return x->Text.GetUtf8() < y->Text.GetUtf8();
            });

        for (auto lemmaPtr : lemmaPtrs) {
            SerializeToProto(*lemmaPtr, *proto.AddLemmas());
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TBlockData& blockData, NReqBundleProtocol::TBlock& proto) const
    {
        proto.SetDistance(blockData.Distance);
        if (blockData.Type != GetDefault(proto).GetType()) {
            proto.SetType(blockData.Type);
        }

        for (const NDetail::TWordData& wordData : blockData.Words) {
            SerializeToProto(wordData, *proto.AddWords());
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TBinaryBlockData& binaryData, NReqBundleProtocol::TBlock& proto) const
    {
        proto.SetBinaryData(binaryData.Data.Data(), binaryData.Data.Size());
        proto.SetBinaryHash(binaryData.Hash);
    }

    void TSerializer::SerializeToProto(const NDetail::TSequenceData& seqData, NReqBundleProtocol::TReqBundle& proto,
        NDetail::TBlocksRemapper* remap) const
    {
        TVector<TSequenceElemDataWithIndex> blocks;
        blocks.reserve(seqData.Elems.size());

        if (!!remap) {
            remap->Reset(seqData.Elems.size());
        }

        for (size_t i : xrange(seqData.Elems.size())) {
            blocks.emplace_back(&seqData.Elems[i], i);
        }

        if (Options.OrderBlocks) {
            StableSort(blocks.begin(), blocks.end());
        }

        size_t newIndex = 0;
        for (const auto& elemDataWithIndex : blocks) {
            const auto& elemData = *elemDataWithIndex.Ptr;

            if (!!remap) {
                (*remap)[elemDataWithIndex.Index] = newIndex++;
            }

            NReqBundleProtocol::TBlock& protoBlock = *proto.AddBlocks();
            if (EnforceBinary == Options.BinMode) {
                TBinaryBlock binary;
                MakeBinary(*elemData.Block, binary);
                SerializeToProto(binary, protoBlock);
            } else {
                if (elemData.Binary && (PreferBinary == Options.BinMode || !elemData.Block)) {
                    SerializeToProto(*elemData.Binary, protoBlock);
                } else {
                    SerializeToProto(*elemData.Block, protoBlock);
                }
            }
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TMatchData& matchData, NReqBundleProtocol::TRequestMatch& proto,
        NDetail::TBlocksRemapper* remap) const
    {
        proto.MutableBlockMatch()->SetBlockIndex(!!remap ? (*remap)[matchData.BlockIndex] : matchData.BlockIndex);
        if (matchData.AnchorWordsRange) {
            proto.MutableBlockMatch()->SetAnchorWordsRangeBegin(matchData.AnchorWordsRange->first);
            proto.MutableBlockMatch()->SetAnchorWordsRangeEnd(matchData.AnchorWordsRange->second);
        }
        proto.MutableWordMatch()->SetWordIndexFirst(matchData.WordIndexFirst);
        if (matchData.WordIndexLast != matchData.WordIndexFirst) {
            proto.MutableWordMatch()->SetWordIndexLast(matchData.WordIndexLast);
        }

        if (static_cast<ui32>(matchData.Type) != GetDefault(proto).GetInfo().GetMatchType()) {
            proto.MutableInfo()->SetMatchType(static_cast<ui32>(matchData.Type));
        }
        if (matchData.SynonymMask != GetDefault(proto).GetInfo().GetSynonymMask()) {
            proto.MutableInfo()->SetSynonymMask(matchData.SynonymMask);
        }
        if (matchData.Weight != GetDefault(proto).GetInfo().GetWeight()) {
            proto.MutableInfo()->SetWeight(matchData.Weight);
        }
        TRevFreqsArrayProto freqsProto;
        SerializeToProto(matchData.RevFreq, freqsProto);
        if (freqsProto.size() > 0) {
            proto.MutableInfo()->MutableRevFreqsByType()->Swap(&freqsProto);
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TConstraintData& constraintData, NReqBundleProtocol::TConstraint& proto,
        NDetail::TBlocksRemapper* remap) const
    {
        if (static_cast<ui32>(constraintData.Type) != GetDefault(proto).GetType()) {
            proto.SetType(static_cast<ui32>(constraintData.Type));
        }

        ::google::protobuf::RepeatedField<ui32>* blockIndicesField = proto.MutableBlockIndices();
        blockIndicesField->Reserve(constraintData.BlockIndices.size());
        if (remap) {
            for (size_t i = 0; i < constraintData.BlockIndices.size(); i++) {
                blockIndicesField->Add((*remap)[constraintData.BlockIndices[i]]);
            }
        } else {
            for (size_t i = 0; i < constraintData.BlockIndices.size(); i++) {
                blockIndicesField->Add(constraintData.BlockIndices[i]);
            }
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TFacetsData::TEntry& entryData, NReqBundleProtocol::TFacet& proto) const
    {
        if (entryData.Id.IsValid<EFacetPartType::Expansion>()) {
            proto.SetExpansionType(static_cast<ui32>(entryData.Id.Get<EFacetPartType::Expansion>()));
        }
        if (entryData.Id.IsValid<EFacetPartType::RegionId>()) {
            proto.SetRegionId(entryData.Id.Get<EFacetPartType::RegionId>().Value);
        }
        if (entryData.Value != GetDefault(proto).GetValue()) {
            proto.SetValue(entryData.Value);
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TRequestData& reqData, NReqBundleProtocol::TRequest& proto,
        NDetail::TBlocksRemapper* remap) const
    {
        proto.SetNumWords(reqData.Words.size());
        if (reqData.AnchorsSrcLength > 0) {
            proto.SetAnchorsSrcLength(reqData.AnchorsSrcLength);
        }

        for (auto& matchData : reqData.Matches) {
            SerializeToProto(matchData, *proto.AddMatches(), remap);
        }

        for (size_t j : xrange(reqData.Proxes.size())) {
            auto& proxData = reqData.Proxes[j];
            if (proxData.Cohesion != GetDefault<NReqBundleProtocol::TWordProximity>().GetCohesion()) {
                while (j >= proto.ProxesSize()) {
                    proto.AddProxes();
                }
                proto.MutableProxes(j)->SetCohesion(proxData.Cohesion);
            }
            if (proxData.Multitoken != GetDefault<NReqBundleProtocol::TWordProximity>().GetMultitoken()) {
                while (j >= proto.ProxesSize()) {
                    proto.AddProxes();
                }
                proto.MutableProxes(j)->SetMultitoken(proxData.Multitoken);
            }
        }

        for (size_t j : xrange(reqData.Words.size())) {
            auto& wordData = reqData.Words[j];
            TRevFreqsArrayProto freqsProto;
            SerializeToProto(wordData.RevFreq, freqsProto);
            if (freqsProto.size() > 0) {
                while (j >= proto.WordsSize()) {
                    proto.AddWords();
                }
                proto.MutableWords(j)->MutableRevFreqsByType()->Swap(&freqsProto);
            }

            if (!wordData.Token.Empty()) {
                while (j >= proto.WordsSize()) {
                    proto.AddWords();
                }
                proto.MutableWords(j)->SetToken(wordData.Token.GetUtf8());
            }
        }

        bool isLegacy = false;
        if (reqData.Facets.Entries.size() == 1) {
            const auto& entry = reqData.Facets.Entries[0];
            EExpansionType expType = entry.Id.Get<EFacetPartType::Expansion>();
            if (entry.Id == MakeFacetId(expType)) {
                isLegacy = true;
                proto.SetExpansionType(static_cast<size_t>(expType));
                proto.SetWeight(entry.Value);
            }
        }

        if (!isLegacy) {
            for (auto& entryData : reqData.Facets.Entries) {
                SerializeToProto(entryData, *proto.AddFacets());
            }
        }

        if (reqData.TrCompatibilityInfo.Defined()) {
            SerializeToProto(*reqData.TrCompatibilityInfo, *proto.MutableTrCompatibilityInfo());
        }
    }

    void TSerializer::SerializeToProto(const TRequestTrCompatibilityInfo& info, NReqBundleProtocol::TRequestTrCompatibilityInfo& proto) const
    {
        for (ui64 wordMask : info.MainPartsWordMasks) {
            proto.AddMainPartsWordMasks(wordMask);
        }
        for (EFormClass formClass : info.MarkupPartsBestFormClasses) {
            proto.AddMarkupPartsBestFormClasses(static_cast<ui32>(formClass));
        }
        proto.SetWordCount(info.WordCount);

        if (info.TopAndArgsForWeb) {
            const TTopAndArgsForWeb& topAndArgs = *info.TopAndArgsForWeb;
            NReqBundleProtocol::TTopAndArgsForWeb& topAndArgsProto = *proto.MutableTopAndArgsForWeb();

            for (const TProximity& proximity : topAndArgs.Proxes) {
                proximity.ToProto(*topAndArgsProto.AddProxes());
            }

            topAndArgsProto.SetIsPosRestrictsEmpty(topAndArgs.IsPosRestrictsEmpty);

            for (const auto& hitInfo : topAndArgs.HitInfos) {
                hitInfo.ToProto(*topAndArgsProto.AddHitInfos());
            }

            for (const bool isStopWord : topAndArgs.IsStopWord) {
                topAndArgsProto.AddIsStopWord(isStopWord);
            }

            for (const bool isPlusWord : topAndArgs.IsPlusWord) {
                topAndArgsProto.AddIsPlusWord(isPlusWord);
            }
        }
    }

    void TSerializer::SerializeToProto(const NDetail::TReqBundleData& bundleData, NReqBundleProtocol::TReqBundle& proto) const
    {
        NDetail::TBlocksRemapper remap;
        SerializeToProto(*bundleData.Sequence, proto, Options.OrderBlocks ? &remap : nullptr);

        for (auto& request : bundleData.Requests) {
            SerializeToProto(*request, *proto.AddRequests(), Options.OrderBlocks ? &remap : nullptr);
        }

        for (auto& constraint : bundleData.Constraints) {
            SerializeToProto(*constraint, *proto.AddConstraints(), Options.OrderBlocks ? &remap : nullptr);
        }
    }

    void TSerializer::SerializeToProto(TConstBlockAcc block, NReqBundleProtocol::TBlock& proto) const
    {
        SerializeToProto(NDetail::BackdoorAccess(block), proto);
    }

    void TSerializer::SerializeToProto(TConstBinaryBlockAcc binary, NReqBundleProtocol::TBlock& proto) const
    {
        SerializeToProto(NDetail::BackdoorAccess(binary), proto);
    }

    void TSerializer::SerializeToProto(TConstSequenceAcc seq, NReqBundleProtocol::TReqBundle& proto,
        NDetail::TBlocksRemapper* remap) const
    {
        SerializeToProto(NDetail::BackdoorAccess(seq), proto, remap);
    }

    void TSerializer::SerializeToProto(TConstRequestAcc request, NReqBundleProtocol::TRequest& proto,
        NDetail::TBlocksRemapper* remap) const
    {
        SerializeToProto(NDetail::BackdoorAccess(request), proto, remap);
    }

    void TSerializer::SerializeToProto(TConstConstraintAcc constraint, NReqBundleProtocol::TConstraint& proto,
        NDetail::TBlocksRemapper *remap) const
    {
        SerializeToProto(NDetail::BackdoorAccess(constraint), proto, remap);
    }

    void TSerializer::SerializeToProto(TConstReqBundleAcc bundle, NReqBundleProtocol::TReqBundle& proto) const
    {
        SerializeToProto(NDetail::BackdoorAccess(bundle), proto);
    }

    TString TSerializer::SerializeBundle(TConstReqBundleAcc bundle) const {
        return Serialize<TConstReqBundleAcc>(bundle);
    }

    void TSerializer::Serialize(TConstBlockAcc block, IOutputStream* output) const
    {
        SerializeHelper<TConstBlockAcc, NReqBundleProtocol::TBlock>(block, output);
    }

    void TSerializer::Serialize(TConstBinaryBlockAcc binary, IOutputStream* output) const
    {
        SerializeHelper<TConstBinaryBlockAcc, NReqBundleProtocol::TBlock>(binary, output);
    }

    void TSerializer::Serialize(TConstSequenceAcc seq, IOutputStream* output) const
    {
        SerializeHelper<TConstSequenceAcc, NReqBundleProtocol::TReqBundle>(seq, output);
    }

    void TSerializer::Serialize(TConstRequestAcc request, IOutputStream* output) const
    {
        SerializeHelper<TConstRequestAcc, NReqBundleProtocol::TRequest>(request, output);
    }

    void TSerializer::Serialize(TConstReqBundleAcc bundle, IOutputStream* output) const
    {
        SerializeHelper<TConstReqBundleAcc, NReqBundleProtocol::TReqBundle>(bundle, output);
    }

    void TSerializer::PrepareRawData(TConstBlockAcc block, TBuffer& buffer, size_t& hash) const
    {
        buffer.Clear();

        NReqBundleProtocol::TBlock blockProto;
        SerializeToProto(block, blockProto);
        TBufferOutput bufOutput(buffer);
        blockProto.SerializeToArcadiaStream(&bufOutput);

        hash = CityHash64(buffer.Data(), buffer.Size());
    }

    void TSerializer::PrepareRawData(TConstRequestAcc request, bool ignoreFacets, TBuffer& buffer, size_t& hash) const
    {
        buffer.Clear();

        NReqBundleProtocol::TRequest requestProto;
        SerializeToProto(request, requestProto);

        if (ignoreFacets) {
            requestProto.ClearFacets();
            requestProto.ClearExpansionType();
            requestProto.ClearWeight();
        }
        TBufferOutput bufOutput(buffer);
        requestProto.SerializeToArcadiaStream(&bufOutput);

        hash = CityHash64(buffer.Data(), buffer.Size());
    }

    size_t TSerializer::GetBinaryHash(TConstBlockAcc block) const
    {
        if (Buffer.Capacity() < DefaultBufferSize) {
            Buffer.Reserve(DefaultBufferSize);
        }

        size_t hash = 0;
        PrepareRawData(block, Buffer, hash);
        Buffer.Clear();
        return hash;
    }

    size_t TSerializer::GetBinaryHash(TConstRequestAcc request, bool ignoreFacets) const
    {
        if (Buffer.Capacity() < DefaultBufferSize) {
            Buffer.Reserve(DefaultBufferSize);
        }

        size_t hash = 0;
        PrepareRawData(request, ignoreFacets, Buffer, hash);
        Buffer.Clear();
        return hash;
    }

    void TSerializer::MakeBinary(TConstBlockAcc block, TBinaryBlockAcc binary) const
    {
        if (Buffer.Capacity() < DefaultBufferSize) {
            Buffer.Reserve(DefaultBufferSize);
        }

        auto& binaryContents = NDetail::BackdoorAccess(binary);
        PrepareRawData(block, Buffer, binaryContents.Hash);

        TBuffer resultBuffer;

        {
            TBufferOutput resultBufOutput(resultBuffer);
            TCompressorFactory::Compress(&resultBufOutput, TStringBuf(Buffer.data(), Buffer.size()), Options.BlocksFormat);
        }

        binaryContents.Data = TBlob::FromBuffer(resultBuffer);
    }

    //
    //

    TString TDeserializer::GetFullErrorMessage() const {
        return Handler.GetFullErrorMessage(TStringBuf("<deserializer message> "));
    }

    void TDeserializer::ValidateRevFreq(i64& revFreq) const {
        if(!NLingBoost::IsValidRevFreq(revFreq)) {
            Handler.Error(yexception() << "bad reverse freq value, " << revFreq);
            revFreq = NLingBoost::InvalidRevFreq;
        }
        revFreq = NLingBoost::CanonizeRevFreq(revFreq);
    }

    void TDeserializer::ValidateAndDecodeLanguage(ui32 code, ELanguage& lang) const {
        static_assert(LANG_UNK == 0, "");
        if (static_cast<i64>(code) >= LANG_MAX) {
            Handler.Error(yexception() << "bad language index value, " << code);
            lang = LANG_UNK;
        } else {
            lang = static_cast<ELanguage>(code);
        }
    }

    void TDeserializer::ValidateAndDecodeType(ui32 code, NDetail::EBlockType& type) const {
        if (code != NDetail::EBlockType::Unordered
            && code != NDetail::EBlockType::ExactOrdered)
        {
            Handler.Error(yexception() << "bad block type, " << code);
            type = NDetail::EBlockType::Unordered;
        } else {
            type = static_cast<NDetail::EBlockType>(code);
        }
    }

    void TDeserializer::ValidateAndDecodeCaseFlags(ui32 code, TCharCategory& cc) const
    {
        if (code > CC_WHOLEMASK) {
            Handler.Error(yexception() << "bad char category mask, " << Hex(code));
            cc = CC_EMPTY;
        } else {
            cc = static_cast<TCharCategory>(code);
        }
    }

    void TDeserializer::ValidateAndDecodeNlpType(ui32 code, NLP_TYPE& nlp) const
    {
        if (code != NLP_WORD
            && code != NLP_INTEGER
            && code != NLP_FLOAT
            && code != NLP_MARK
            && code != NLP_END)
        {
            Handler.Error(yexception() << "bad NLP type, " << code);
            nlp = NLP_WORD;
        } else {
            nlp = static_cast<NLP_TYPE>(code);
        }
    }

    void TDeserializer::ValidateAndDecodeConstraintType(ui32 code, EConstraintType& type) const
    {
        if (!NLingBoost::TConstraint::HasIndex(code)) {
            Handler.Error(yexception() << "bad constraint type, " << code);
            type = EConstraintType::Must;
        } else {
            type = static_cast<EConstraintType>(code);
        }
    }

    void TDeserializer::ValidateAndDecodeFormClass(ui32 code, EFormClass& formClass) const
    {
        if (code >= NUM_FORM_CLASSES) {
            Handler.Error(yexception() << "bad form class, " << code);
            formClass = EQUAL_BY_SYNONYM;
        } else {
            formClass = static_cast<EFormClass>(code);
        }
    }

    void TDeserializer::ValidateMatchWeight(double& weight) const
    {
        if (weight < 0.0f) {
            // Currently may contain non-normalized
            // floats (synonym relev from rich tree).
            // Disable error messages.
            //
            // Handler.Error(yexception() << "bad match weight, " << weight);
            // weight = 0.0f;
            // TODO: Uncommect and canonize diffs
        }
    }

    void TDeserializer::ValidateFacetValue(float& value) const
    {
        value = SoftClipFloatTo01(value, 1e-2);
        if (value < 0.0f || value > 1.0f) {
            Handler.Error(yexception() << "bad facet value, " << value);
            value = 0.0f;
        }
    }

    void TDeserializer::ValidateCohesion(float& cohesion) const
    {
        if (cohesion < 0.0f || cohesion > 1.0f) {
            Handler.Error(yexception() << "bad cohesion, " << cohesion);
            cohesion = 0.0f;
        }
    }

    void TDeserializer::ValidateRegionId(i64& regionId) const {
        if (regionId < 0) {
            Handler.Error(yexception() << "bad region id, " << regionId);
            regionId = TRegionId::World();
        }
    }

    void TDeserializer::DeserializeProto(const TRevFreqsArrayProto& proto, NLingBoost::TRevFreq& revFreq) const
    {
        for (int valueIndex = 0; valueIndex < proto.size(); ++valueIndex) {
            const size_t freqIndex = proto.Get(valueIndex).GetFreqType();

            if (!TWordFreq::HasIndex(freqIndex)) {
                Handler.Error(yexception() << "bad freq type index, " << freqIndex);
                continue;
            }
            const EWordFreqType freqType = static_cast<EWordFreqType>(freqIndex);
            revFreq.Values[freqType] = proto.Get(valueIndex).GetRevFreq();
            ValidateRevFreq(revFreq.Values[freqType]);
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlockFormInfo& proto, size_t prefixSize, const NDetail::TFormData& prevFormData, NDetail::TFormData& formData) const
    {
        formData.Text = prevFormData.Text.GetUtf8().substr(0, prefixSize) + proto.GetText();
        formData.Exact = proto.GetExact();
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlockLemmaInfo& proto, NDetail::TLemmaData& lemmaData) const
    {
        lemmaData.Text = proto.GetText();
        lemmaData.Best = proto.GetBest();
        lemmaData.Attribute = proto.GetAttribute();
        ValidateAndDecodeLanguage(proto.GetLanguage(), lemmaData.Language);

        lemmaData.RevFreq.Values.Fill(NLingBoost::InvalidRevFreq);
        if (proto.HasRevFreq()) {
            lemmaData.RevFreq.Values[NLingBoost::TWordFreq::Default] = proto.GetRevFreq();
            ValidateRevFreq(lemmaData.RevFreq.Values[NLingBoost::TWordFreq::Default]);
        }
        DeserializeProto(proto.GetRevFreqsByType(), lemmaData.RevFreq);

        size_t numForms = proto.FormsSize();
        if (numForms > TSizeLimits::MaxNumForms) {
            Handler.Error(yexception() << "too many forms in lemma, " << numForms);
            numForms = TSizeLimits::MaxNumForms;
        }

        size_t prefixSizes = proto.FormsPrefixSizesSize();
        if (prefixSizes != 0 && prefixSizes + 1 != numForms) {
            Handler.Error(yexception() << "prefixSizes are not as same as numForms - 1 " << prefixSizes << ' ' << numForms);
        }

        lemmaData.Forms.resize(numForms);
        for (size_t formIndex : xrange(numForms)) {
            TErrorGuard guardForm{Handler, "form", formIndex};
            if (formIndex && formIndex < prefixSizes + 1) {
                DeserializeProto(proto.GetForms(formIndex), proto.GetFormsPrefixSizes(formIndex - 1), lemmaData.Forms[formIndex -  1], lemmaData.Forms[formIndex]);
            } else {
                DeserializeProto(proto.GetForms(formIndex), 0, NDetail::TFormData{}, lemmaData.Forms[formIndex]);
            }
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlockWordInfo& proto, NDetail::TWordData& wordData) const
    {
        wordData.Text = proto.GetText();
        wordData.LangMask = ::Deserialize(proto.GetLanguages());
        wordData.StopWord = proto.GetStopWord();
        wordData.AnyWord = proto.GetAnyWord();
        ValidateAndDecodeCaseFlags(proto.GetCaseFlags(), wordData.CaseFlags);
        ValidateAndDecodeNlpType(proto.GetNlpType(), wordData.NlpType);

        wordData.RevFreq.Values.Fill(NLingBoost::InvalidRevFreq);
        if (proto.HasRevFreq()) {
            wordData.RevFreq.Values[NLingBoost::TWordFreq::Default] = proto.GetRevFreq();
            ValidateRevFreq(wordData.RevFreq.Values[NLingBoost::TWordFreq::Default]);
        }
        DeserializeProto(proto.GetRevFreqsByType(), wordData.RevFreq);

        wordData.RevFreqAllForms.Values.Fill(NLingBoost::InvalidRevFreq);
        if (proto.HasRevFreqAllForms()) {
            wordData.RevFreqAllForms.Values[NLingBoost::TWordFreq::Default] = proto.GetRevFreqAllForms();
            ValidateRevFreq(wordData.RevFreqAllForms.Values[NLingBoost::TWordFreq::Default]);
        }
        DeserializeProto(proto.GetRevFreqsAllFormsByType(), wordData.RevFreqAllForms);

        size_t numLemmas = proto.LemmasSize();
        if (numLemmas > TSizeLimits::MaxNumLemmas) {
            Handler.Error(yexception() << "too many lemmas in word, " << numLemmas);
            numLemmas = TSizeLimits::MaxNumLemmas;
        }

        wordData.Lemmas.resize(numLemmas);
        for (size_t lemmaIndex : xrange(numLemmas)) {
            TErrorGuard guardLemma{Handler, "lemma", lemmaIndex};

            DeserializeProto(proto.GetLemmas(lemmaIndex), wordData.Lemmas[lemmaIndex]);
        }

        if (wordData.NlpType == NLP_END) { // TODO(sankear): REMOVE THIS HACK AFTER WIZARD'S RELEASE, TICKET SEARCH-4000
            for (NDetail::TLemmaData& lemmaData : wordData.Lemmas) {
                static const TUtf16String Separator = u"=";
                if (!lemmaData.Attribute && lemmaData.Forms.empty() && lemmaData.Text.GetWide().find(Separator) != TUtf16String::npos) {
                    lemmaData.Attribute = true;
                }
            }
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlock& proto, NDetail::TBlockData& blockData) const
    {
        blockData.Distance = proto.GetDistance(); // very large values are suspicious, but not invalid
        ValidateAndDecodeType(proto.GetType(), blockData.Type);

        size_t numWordsInBlock = proto.WordsSize();
        if (numWordsInBlock > TSizeLimits::MaxNumWordsInBlock) {
            Handler.Error(yexception() << "too many words in block, " << numWordsInBlock);
            numWordsInBlock = TSizeLimits::MaxNumWordsInBlock;
        }

        blockData.Words.resize(numWordsInBlock);
        for (size_t wordIndex : xrange(numWordsInBlock)) {
            TErrorGuard guardWord{Handler, "word", wordIndex};

            DeserializeProto(proto.GetWords(wordIndex), blockData.Words[wordIndex]);
        }
        auto&& anyWordPred = [](const NDetail::TWordData& word) -> bool { return word.AnyWord; };
        if ((blockData.Type != NDetail::EBlockType::ExactOrdered) && !!FindIfPtr(blockData.Words, anyWordPred)) {
            Handler.Error(yexception() << "any word in block, " << static_cast<ui32>(blockData.Type));
            blockData.Words.erase(std::remove_if(blockData.Words.begin(), blockData.Words.end(), anyWordPred), blockData.Words.end());
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlock& proto, NDetail::TBinaryBlockData& binaryData) const
    {
        auto& bytes = proto.GetBinaryData();
        binaryData.Data = TBlob::Copy(bytes.data(), bytes.size());
        binaryData.Hash = proto.GetBinaryHash();
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TRequestTrCompatibilityInfo& proto, TRequestTrCompatibilityInfo& info) const
    {
        TErrorGuard guardCompatibilityInfo{Handler, "tr_info"};
        size_t numMainPartsWordMasks = proto.MainPartsWordMasksSize();
        if (numMainPartsWordMasks > TSizeLimits::MaxMainPartsWordMasks) {
            Handler.Error(yexception() << "too many main parts word masks, " << numMainPartsWordMasks);
            numMainPartsWordMasks = TSizeLimits::MaxMainPartsWordMasks;
        }
        info.MainPartsWordMasks.resize(numMainPartsWordMasks);
        for (size_t i = 0; i < numMainPartsWordMasks; ++i) {
            info.MainPartsWordMasks[i] = proto.GetMainPartsWordMasks(i);
        }
        size_t numMarkupPartsBestFormClasses = proto.MarkupPartsBestFormClassesSize();
        if (numMarkupPartsBestFormClasses > TSizeLimits::MaxMarkupPartsBestFormClasses) {
            Handler.Error(yexception() << "too many markup parts best form classes, " << numMarkupPartsBestFormClasses);
            numMarkupPartsBestFormClasses = TSizeLimits::MaxMarkupPartsBestFormClasses;
        }
        info.MarkupPartsBestFormClasses.resize(numMarkupPartsBestFormClasses);
        for (size_t i = 0; i < numMarkupPartsBestFormClasses; ++i) {
            ValidateAndDecodeFormClass(proto.GetMarkupPartsBestFormClasses(i), info.MarkupPartsBestFormClasses[i]);
        }
        info.WordCount = proto.GetWordCount();

        if (proto.HasTopAndArgsForWeb()) {
            const NReqBundleProtocol::TTopAndArgsForWeb& topAndArgsProto = proto.GetTopAndArgsForWeb();
            info.TopAndArgsForWeb.ConstructInPlace();
            TTopAndArgsForWeb& topAndArgs = *info.TopAndArgsForWeb;

            topAndArgs.Proxes.resize(topAndArgsProto.ProxesSize());
            for (ui32 pr = 0; pr < topAndArgsProto.ProxesSize(); ++pr) {
                topAndArgs.Proxes[pr].FromProto(topAndArgsProto.GetProxes(pr));
            }

            topAndArgs.IsPosRestrictsEmpty = topAndArgsProto.GetIsPosRestrictsEmpty();

            topAndArgs.HitInfos.resize(topAndArgsProto.HitInfosSize());
            for (ui32 hit = 0; hit < topAndArgsProto.HitInfosSize(); ++hit) {
                topAndArgs.HitInfos[hit].FromProto(topAndArgsProto.GetHitInfos(hit));
            }

            for (const bool isStopWord : topAndArgsProto.GetIsStopWord()) {
                topAndArgs.IsStopWord.push_back(isStopWord);
            }

            for (const bool isPlusWord : topAndArgsProto.GetIsPlusWord()) {
                topAndArgs.IsPlusWord.push_back(isPlusWord);
            }
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, NDetail::TSequenceData& seqData) const
    {
        TErrorGuard guardSequence{Handler, "sequence"};

        size_t numBlocks = proto.BlocksSize();
        if (numBlocks > TSizeLimits::MaxNumBlocks) {
            Handler.Error(yexception() << "too many basic blocks, " << numBlocks);
            numBlocks = TSizeLimits::MaxNumBlocks;
        }

        for (size_t i : xrange(numBlocks)) {
            TErrorGuard guardBlock{Handler, "elem", i};

            const NReqBundleProtocol::TBlock& protoBlock = proto.GetBlocks(i);

            if (protoBlock.HasBinaryData()) {
                THolder<TBinaryBlock> binary{new TBinaryBlock};
                DeserializeProto(protoBlock, *binary);
                seqData.Elems.push_back(NDetail::SequenceElem(binary.Release()));
            } else {
                THolder<TBlock> block{new TBlock};
                DeserializeProto(protoBlock, *block);
                seqData.Elems.push_back(NDetail::SequenceElem(block.Release()));
            }
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TRequestMatch& proto, NDetail::TMatchData& matchData) const
    {
        matchData.BlockIndex = proto.GetBlockMatch().GetBlockIndex();
        matchData.WordIndexFirst = proto.GetWordMatch().GetWordIndexFirst();

        if (proto.GetBlockMatch().HasAnchorWordsRangeBegin()) {
            matchData.AnchorWordsRange = {
                proto.GetBlockMatch().GetAnchorWordsRangeBegin(),
                proto.GetBlockMatch().GetAnchorWordsRangeEnd()
            };
        }

        if (proto.GetWordMatch().HasWordIndexLast()) {
            matchData.WordIndexLast = proto.GetWordMatch().GetWordIndexLast();
        } else {
            matchData.WordIndexLast = matchData.WordIndexFirst;
        }
        matchData.Type = static_cast<EMatchType>(proto.GetInfo().GetMatchType());
        matchData.SynonymMask = proto.GetInfo().GetSynonymMask();

        matchData.RevFreq.Values.Fill(NLingBoost::InvalidRevFreq);
        if (proto.GetInfo().HasRevFreq()) {
            matchData.RevFreq.Values[NLingBoost::TWordFreq::Default] = proto.GetInfo().GetRevFreq();
            ValidateRevFreq(matchData.RevFreq.Values[NLingBoost::TWordFreq::Default]);
        }
        DeserializeProto(proto.GetInfo().GetRevFreqsByType(), matchData.RevFreq);

        matchData.Weight = proto.GetInfo().GetWeight();
        ValidateMatchWeight(matchData.Weight);
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TConstraint& proto, NDetail::TConstraintData& constraintData) const
    {
        ValidateAndDecodeConstraintType(proto.GetType(), constraintData.Type);
        constraintData.BlockIndices.assign(proto.GetBlockIndices().begin(), proto.GetBlockIndices().end());
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TFacet& proto, NDetail::TFacetsData::TEntry& entryData) const
    {
        entryData.Id = MakeFacetId();
        if (proto.HasExpansionType()) {
            Y_ASSERT(TExpansion::HasIndex(proto.GetExpansionType()));
            const EExpansionType expansionType = static_cast<EExpansionType>(proto.GetExpansionType());
            entryData.Id.Set(expansionType);
        }
        if (proto.HasRegionId()) {
            i64 regionId = proto.GetRegionId();
            ValidateRegionId(regionId);
            entryData.Id.Set(NReqBundle::TRegionId(regionId));
        }
        entryData.Value = proto.GetValue();
        ValidateFacetValue(entryData.Value);
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TRequest& proto, NDetail::TRequestData& reqData) const
    {
        const size_t numWords = proto.GetNumWords();
        if (numWords > TSizeLimits::MaxNumWordsInRequest) {
            Handler.Error(yexception() << "too many words in request, " << numWords);
            return;
        }
        if (proto.MatchesSize() > TSizeLimits::MaxNumMatches) {
            Handler.Error(yexception() << "too many matches in request, " << proto.MatchesSize());
            return;
        }

        NDetail::InitRequestData(reqData, numWords);

        reqData.AnchorsSrcLength = proto.GetAnchorsSrcLength();
        reqData.Matches.resize(proto.MatchesSize());
        for (size_t i : xrange(proto.MatchesSize())) {
            TErrorGuard guardMatch{Handler, "match", i};

            const size_t matchIndex = proto.GetMatches(i).GetInfo().GetMatchType();
            if (!TMatch::HasIndex(matchIndex)) {
                Handler.Error(yexception() << "bad match type index, " << matchIndex);
                NDetail::InitRequestData(reqData, 0); // reset to 0 words
                return;
            }
            DeserializeProto(proto.GetMatches(i), reqData.Matches[i]);
            if (auto r = reqData.Matches[i].AnchorWordsRange; r) {
                if (r->first >= r->second || r->second > reqData.AnchorsSrcLength) {
                    Handler.Error(yexception() << "invalid anchors range " <<
                        r->first << ":" << r->second << " length " << reqData.AnchorsSrcLength
                    );

                    reqData.Matches[i].AnchorWordsRange.Clear();
                }
            }
        }

        if (proto.ProxesSize() + 1 > numWords) {
            Handler.Error(yexception() << "too many prox entries, " << proto.ProxesSize() << " with numWords = " << numWords);
        }
        const size_t numProxes = numWords > 0 ? numWords - 1 : 0;
        for (size_t i : xrange(Min(proto.ProxesSize(), numProxes))) {
            TErrorGuard guardProx{Handler, "prox", i};

            auto& proxData = reqData.Proxes[i];
            proxData.Cohesion = proto.GetProxes(i).GetCohesion();
            proxData.Multitoken = proto.GetProxes(i).GetMultitoken();

            ValidateCohesion(proxData.Cohesion);
        }

        if (proto.WordsSize() > numWords) {
            Handler.Error(yexception() << "too many word entries, " << proto.WordsSize() << " with numWords = " << numWords);
        }
        for (size_t i : xrange(Min(proto.WordsSize(), numWords))) {
            TErrorGuard guardWord{Handler, "word", i};

            auto& wordData = reqData.Words[i];
            wordData.RevFreq.Values.Fill(NLingBoost::InvalidRevFreq);
            if (proto.GetWords(i).HasRevFreq()) {
                wordData.RevFreq.Values[NLingBoost::TWordFreq::Default] = proto.GetWords(i).GetRevFreq();
                ValidateRevFreq(wordData.RevFreq.Values[NLingBoost::TWordFreq::Default]);
            }
            DeserializeProto(proto.GetWords(i).GetRevFreqsByType(), wordData.RevFreq);

            if (proto.GetWords(i).HasToken()) {
                wordData.Token = proto.GetWords(i).GetToken();
            }
        }

        size_t numFacets = proto.FacetsSize();
        if (numFacets > TSizeLimits::MaxNumFacets) {
            Handler.Error(yexception() << "too many facets in request, " << numFacets);
            numFacets = TSizeLimits::MaxNumFacets;
        }

        for (size_t i : xrange(numFacets)) {
            TErrorGuard guardFacet{Handler, "facet", i};

            const size_t expansionIndex = proto.GetFacets(i).GetExpansionType();
            if (!TExpansion::HasIndex(expansionIndex)) {
                Handler.Error(yexception() << "bad expansion type index, " << expansionIndex);
                continue;
            }
            reqData.Facets.Entries.emplace_back();
            DeserializeProto(proto.GetFacets(i), reqData.Facets.Entries.back());
        }
        Sort(reqData.Facets.Entries);

        if (proto.HasExpansionType()) {
            TErrorGuard guardFacet{Handler, "facet"};

            const size_t expansionIndex = proto.GetExpansionType();
            if (!TExpansion::HasIndex(expansionIndex)) {
                Handler.Error(yexception() << "bad expansion type index, " << expansionIndex);
            } else {
                TFacetId id = MakeFacetId(static_cast<NLingBoost::EExpansionType>(proto.GetExpansionType()));

                NDetail::TFacetsData::TEntry* entry = nullptr;
                if (proto.FacetsSize() == 0) { // Backward compatibility
                     auto& entries = reqData.Facets.Entries;
                     entries.emplace_back();
                     entry = &entries.back();
                } else {
                    Handler.Error(yexception() << "deprecated expansion type and request facets are present at the same time");
                    auto res = InsertFacet(reqData.Facets, id);
                    if (res.second) {
                        entry = &res.first;
                    }
                }

                if (entry) {
                    entry->Id = id;
                    entry->Value = proto.GetWeight();
                    ValidateFacetValue(entry->Value);
                }
            }
        }

        if (proto.HasTrCompatibilityInfo()) {
            reqData.TrCompatibilityInfo.ConstructInPlace();
            DeserializeProto(proto.GetTrCompatibilityInfo(), *reqData.TrCompatibilityInfo);
        }
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, NDetail::TReqBundleData& bundleData) const
    {
        TErrorGuard guardReqbundle{Handler, "reqbundle"};

        DeserializeProto(proto, NDetail::BackdoorAccess(*bundleData.Sequence));

        size_t numRequests = proto.RequestsSize();
        if (numRequests > TSizeLimits::MaxNumRequests) {
            Handler.Error(yexception() << "too many requests, " << numRequests);
            numRequests = TSizeLimits::MaxNumRequests;
        }

        for (size_t i : xrange(numRequests)) {
            TErrorGuard guardRequest{Handler, "request", i};

            THolder<TRequest> request{new TRequest};
            DeserializeProto(proto.GetRequests(i), NDetail::BackdoorAccess(*request));
            if (!Options.NeedAllData && !NDetail::IsValidRequest(*request, *bundleData.Sequence)) {	
                Handler.Error(yexception() << "bad request");	
                continue;	
            }
            bundleData.Requests.push_back(request.Release());
        }

        size_t numConstraints = proto.ConstraintsSize();
        if (numConstraints > TSizeLimits::MaxNumConstraints) {
            Handler.Error(yexception() << "too many constraints, " << proto.ConstraintsSize());
            numConstraints = TSizeLimits::MaxNumConstraints;
        }
        for (size_t i : xrange(numConstraints)) {
            TErrorGuard guardConstraint{Handler, "constraint", i};

            THolder<TConstraint> constraint = MakeHolder<TConstraint>();
            DeserializeProto(proto.GetConstraints(i), NDetail::BackdoorAccess(*constraint));
            if (!Options.NeedAllData && !NDetail::IsValidConstraint(*constraint, *bundleData.Sequence)) {	
                Handler.Error(yexception() << "bad constraint, " << i);	
                continue;	
            }
            bundleData.Constraints.push_back(constraint.Release());
        }
    }

    TReqBundlePtr TDeserializer::DeserializeBundle(TStringBuf serialized) const {
        TReqBundlePtr bundle = new TReqBundle;
        Deserialize<TReqBundle>(serialized, *bundle);
        return bundle;
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlock& proto, TBlockAcc block) const
    {
        DeserializeProto(proto, NDetail::BackdoorAccess(block));
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TBlock& proto, TBinaryBlockAcc binary) const
    {
        DeserializeProto(proto, NDetail::BackdoorAccess(binary));
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, TSequenceAcc seq) const
    {
        DeserializeProto(proto, NDetail::BackdoorAccess(seq));
    }

    void TDeserializer::DeserializeProto(const NReqBundleProtocol::TReqBundle& proto, TReqBundleAcc bundle) const
    {
        DeserializeProto(proto, NDetail::BackdoorAccess(bundle));
    }

    void TDeserializer::Deserialize(IInputStream* input, TBlockAcc block) const
    {
        DeserializeHelper<TBlockAcc, NReqBundleProtocol::TBlock>(input, block);
    }

    void TDeserializer::Deserialize(IInputStream* input, TBinaryBlockAcc binary) const
    {
        DeserializeHelper<TBinaryBlockAcc, NReqBundleProtocol::TBlock>(input, binary);
    }

    void TDeserializer::Deserialize(IInputStream* input, TSequenceAcc seq) const
    {
        DeserializeHelper<TSequenceAcc, NReqBundleProtocol::TReqBundle>(input, seq);
    }

    void TDeserializer::Deserialize(IInputStream* input, TReqBundleAcc bundle) const
    {
        DeserializeHelper<TReqBundleAcc, NReqBundleProtocol::TReqBundle>(input, bundle);
    }

    void TDeserializer::ParseBinary(TConstBinaryBlockAcc binary, TBlockAcc block) const
    {
        TErrorGuard binaryGuard{Handler, "binary"};

        TMemoryInput memInput(binary.GetData().Data(), binary.GetData().Size());
        try {
            Deserialize(&memInput, block);
        } catch (...) {
            Handler.Error(yexception() << "failed to parse binary block"
                << "\n<parse error> " << CurrentExceptionMessage());
        }
    }
} // NSer
} // NReqBundle

#include "hits_serializer.h"
#include "hits_serializer_offroad.h"

#include <library/cpp/offroad/custom/null_vectorizer.h>
#include <library/cpp/offroad/custom/ui32_vectorizer.h>
#include <library/cpp/offroad/flat/flat_searcher.h>
#include <library/cpp/offroad/streams/bit_output.h>
#include <library/cpp/resource/resource.h>

#include <util/string/builder.h>

using namespace NTextMachineProtocol;

namespace {
    using namespace NTextMachine;

    using TErrorGuard = NLingBoost::TErrorHandler::TGuard;

    class TBadHit : public TDeserializerError {}; // throw this to correctly skip invalid hits

    const TAnnotation* LookupAnnotation(
        const THitsAuxData& data,
        EStreamType streamType,
        const size_t breakNum,
        const size_t streamIndex)
    {
        auto annotations = data.AnnotationsPtr(streamType);

        if (!annotations) {
            return nullptr;
        }

        auto it = LowerBound(annotations->begin(), annotations->end(),
            std::pair<size_t, size_t>(breakNum, streamIndex),
            [] (const TAnnotation& ann, std::pair<size_t, size_t> val) {
             return std::pair<size_t, size_t>(ann.BreakNumber, ann.StreamIndex) < val;
            });

        const bool isFound = (it != annotations->end()
            && it->BreakNumber == breakNum
            && it->StreamIndex == streamIndex);

        if (!isFound) {
            return nullptr;
        }

        return &*it;
    }

    struct TTextMachineHitReaderTableV1 : public TTextMachineHitReader::TTable {
        TTextMachineHitReader::TModel Model;
        TTextMachineHitReaderTableV1() {
            Model.Load(TBlob::FromString(NResource::Find("text_machine_hit_model_v1")));
            Reset(Model);
        }
    };

    struct TTextMachineHitWriterTableV1 : public TTextMachineHitWriter::TTable {
        TTextMachineHitWriter::TModel Model;
        TTextMachineHitWriterTableV1() {
            Model.Load(TBlob::FromString(NResource::Find("text_machine_hit_model_v1")));
            Reset(Model);
        }
    };

    struct TTextMachineAnnotationReaderTableV1 : public TTextMachineAnnotationReader::TTable {
        TTextMachineAnnotationReader::TModel Model;
        TTextMachineAnnotationReaderTableV1() {
            Model.Load(TBlob::FromString(NResource::Find("text_machine_annotation_model_v1")));
            Reset(Model);
        }
    };

    struct TTextMachineAnnotationWriterTableV1 : public TTextMachineAnnotationWriter::TTable {
        TTextMachineAnnotationWriter::TModel Model;
        TTextMachineAnnotationWriterTableV1() {
            Model.Load(TBlob::FromString(NResource::Find("text_machine_annotation_model_v1")));
            Reset(Model);
        }
    };

} // namespace

namespace NTextMachine {
    THitsSerializer::THitsSerializer(NTextMachineProtocol::TPbHits* hitsBuf, TOptions options)
        : HitsBuf(hitsBuf)
    {
        Y_ASSERT(HitsBuf);
        if (options.UseOffroadCompression) {
            OffroadHitStream = MakeHolder<TStringOutput>(*HitsBuf->MutablePackedBlockHits());
            OffroadHitWriter = MakeHolder<TTextMachineHitWriter>();
            OffroadHitWriter->Reset(HugeSingleton<TTextMachineHitWriterTableV1>(), OffroadHitStream.Get());
            OffroadAnnotationStream = MakeHolder<TStringOutput>(*HitsBuf->MutablePackedAnnotations());
            OffroadAnnotationWriter = MakeHolder<TTextMachineAnnotationWriter>();
            OffroadAnnotationWriter->Reset(HugeSingleton<TTextMachineAnnotationWriterTableV1>(), OffroadAnnotationStream.Get());
            HitsBuf->SetPackVersion(1);
        }
    }

    THitsSerializer::~THitsSerializer()
    {
        Finish();
    }

    void THitsSerializer::ConvertToProto(const TAnnotation& src, TPbAnnotation& dst)
    {
       dst.SetBreakNumber(src.BreakNumber);
       dst.SetFirstWordPos(src.FirstWordPos);
       dst.SetLength(src.Length);
       dst.SetValue(src.Value);
       dst.SetText(src.Text);
       dst.SetLanguage(src.Language);

       if (src.StreamIndex != 0) {
           dst.SetStreamIndex(src.StreamIndex);
       }
    }

    void THitsSerializer::ConvertToOffroad(const TAnnotation& src, TTextMachineAnnotationHit& dst)
    {
        dst.BreakNumber = src.BreakNumber;
        dst.StreamIndex = src.StreamIndex;
        dst.FirstWordPos = src.FirstWordPos;
        dst.Length = src.Length;
        dst.Weight = src.Value;
        // ignore Text and Language
    }

    void THitsSerializer::ConvertToProto(const TStream& src, TPbStream& dst)
    {
        dst.SetType(static_cast<size_t>(src.Type));
        dst.SetAnnotationCount(src.AnnotationCount);
        dst.SetWordCount(src.WordCount);
        dst.SetMaxValue(src.MaxValue);
    }

    void THitsSerializer::ConvertToProto(const TPosition& src, TPbPosition& dst)
    {
        Y_ASSERT(src.Annotation);
        Y_ASSERT(src.Annotation->Stream);

        dst.SetStreamType(static_cast<size_t>(src.Annotation->Stream->Type));
        dst.SetBreakNumber(src.Annotation->BreakNumber);
        dst.SetLeftWordPos(src.LeftWordPos);
        dst.SetRightWordPos(src.RightWordPos);
        dst.SetRelevance(src.Relevance);

        if (src.Annotation->StreamIndex != 0) {
            dst.SetStreamIndex(src.Annotation->StreamIndex);
        }
    }

    void THitsSerializer::ConvertToProto(const THit& src, TPbWordHit& dst)
    {
       dst.SetRequestIndex(src.Word.QueryId);
       dst.SetWordIndex(src.Word.WordId);
       dst.SetFormIndex(src.Word.FormId);
       dst.SetWeight(src.Weight);
       ConvertToProto(src.Position, *dst.MutablePosition());
    }

    void THitsSerializer::ConvertToProto(const TBlockHit& src, TPbBlockHit& dst)
    {
       dst.SetLayer(static_cast<size_t>(src.Layer));
       dst.SetBlockIndex(src.BlockId);
       dst.SetPrecision(static_cast<size_t>(src.Precision));
       dst.SetLemmaIndex(src.LemmaId);
       dst.SetWeight(src.Weight);
       dst.SetFormIndex(src.FormId);
       dst.SetLowLevelFormIndex(src.LowLevelFormId);
       ConvertToProto(src.Position, *dst.MutablePosition());
    }

    void THitsSerializer::ConvertToOffroad(const TBlockHit& src, TTextMachineHit& dst, ui32 annotationIndex)
    {
        dst.AnnotationIndex = annotationIndex;

        dst.Layer = static_cast<ui32>(src.Layer);
        dst.BlockIndex = src.BlockId;
        dst.Precision = static_cast<ui32>(src.Precision);
        dst.LemmaIndex = src.LemmaId;
        // Weight is not converted
        dst.FormIndex = src.FormId;
        dst.LowLevelFormIndex = src.LowLevelFormId;

        dst.LeftWordPos = src.Position.LeftWordPos;
        Y_ENSURE(src.Position.RightWordPos >= src.Position.LeftWordPos);
        dst.RightWordPosDelta = src.Position.RightWordPos - src.Position.LeftWordPos;
        dst.Relevance = src.Position.Relevance;
    }

    void THitsSerializer::UpdateAuxDataInProto(const TPosition& src, TPbHits& dst)
    {
        Y_ENSURE_EX(src.AnnotationPtr(), TSerializerError{} << "annotation pointer is null");
        Y_ENSURE_EX(src.StreamPtr(), TSerializerError{} << "stream pointer is null");

        const EStreamType streamType = src.StreamRef().Type;
        const size_t streamTypeIndex = static_cast<size_t>(streamType);

        const auto& remap = TStream::GetFullRemap();

        Y_ENSURE_EX(remap.HasKey(streamType),
            TSerializerError{} << "invalid stream type value, " << streamTypeIndex);

        const size_t targetIndex = remap.GetIndex(streamType);
        for (size_t nextIndex = dst.StreamsSize(); nextIndex <= targetIndex; ++nextIndex) {
            const EStreamType nextStreamType = remap.GetKey(nextIndex);
            const size_t nextStreamIndex = static_cast<size_t>(nextStreamType);

            TPbStream& newStreamProto = *dst.AddStreams(); // Add empty streams as needed
            newStreamProto.SetType(nextStreamIndex);
        }

        Y_ASSERT(targetIndex < dst.StreamsSize());
        TPbStream& streamProto = *dst.MutableStreams(targetIndex);
        Y_ASSERT(streamProto.GetType() == streamTypeIndex);

        bool isNewAnnotation = false;

        bool isNewStream = false;
        if (OffroadAnnotationWriter) {
            isNewStream = !streamProto.HasAnnotationCount();
        } else {
            isNewStream = (streamProto.AnnotationsSize() == 0);
        }
        const size_t breakNum = src.Annotation->BreakNumber;
        const size_t streamIndex = src.Annotation->StreamIndex;
        if (isNewStream) {
            ConvertToProto(*src.Annotation->Stream, streamProto);
            isNewAnnotation = true;
        } else {
            if (OffroadAnnotationWriter) {
                TLastAnnotationKey key{streamType, streamIndex};
                TLastAnnotationInStream* last = LastAnnotations.FindPtr(key);
                if (last) {
                    Y_ENSURE_EX(last->BreakNumber <= breakNum,
                        THitsOrderError() << "hits for stream " << src.Annotation->Stream->Type
                            << " and index " << src.Annotation->StreamIndex
                            << " must be ordered by break number, but " << breakNum
                            << " appeared after " << last->BreakNumber);
                    isNewAnnotation = (last->BreakNumber != breakNum);
                } else {
                    isNewAnnotation = true;
                }
            } else {
                const TPbAnnotation& lastAnnProto = streamProto.GetAnnotations(streamProto.AnnotationsSize() - 1);
                const size_t lastBreakNum = lastAnnProto.GetBreakNumber();
                const size_t lastStreamIndex = lastAnnProto.GetStreamIndex();

                Y_ENSURE_EX(lastBreakNum <= breakNum,
                   THitsOrderError() << "hits for stream " << src.Annotation->Stream->Type
                   << " must be ordered by pair (break number, stream index), but " << breakNum
                   << " appeared after " << lastBreakNum);

                isNewAnnotation = (std::make_pair(lastBreakNum, lastStreamIndex)
                    < std::make_pair(breakNum, streamIndex));
                Y_ASSERT(isNewAnnotation == (
                    std::find_if(streamProto.GetAnnotations().begin(),
                        streamProto.GetAnnotations().end(),
                        [=](const TPbAnnotation& a) {
                            return a.GetBreakNumber() == breakNum
                                && a.GetStreamIndex() == streamIndex;}) == streamProto.GetAnnotations().end()));
            }
        }

        if (isNewAnnotation) {
            if (OffroadAnnotationWriter) {
                TLastAnnotationInStream& lastAnn = LastAnnotations[TLastAnnotationKey{streamType, streamIndex}];
                lastAnn.BreakNumber = breakNum;
                lastAnn.AnnotationIndex = AnnotationStreamTypes.size();

                TTextMachineAnnotationHit hit;
                ConvertToOffroad(*src.Annotation, hit);
                OffroadAnnotationWriter->WriteHit(hit);

                if (src.Annotation->Text || src.Annotation->Language) {
                    TPbAnnotation& newAnnProto = *streamProto.AddAnnotations();
                    newAnnProto.SetText(src.Annotation->Text);
                    newAnnProto.SetLanguage(src.Annotation->Language);
                }

                AnnotationStreamTypes.push_back(streamType);
            } else {
                TPbAnnotation& newAnnProto = *streamProto.AddAnnotations();
                ConvertToProto(*src.Annotation, newAnnProto);
            }
        }
    }

    void THitsSerializer::AddWordHit(const THit& hit)
    {
        UpdateAuxDataInProto(hit.Position, *HitsBuf);
        TPbWordHit& hitProto = *HitsBuf->AddWordHits();
        ConvertToProto(hit, hitProto);
    }

    void THitsSerializer::SetJsonsFromDssmTextualFieldSetV1(TString words, TString bigrams) const {
        HitsBuf->SetWordJsonsFromDssmTextualFieldSetV1(words);
        HitsBuf->SetBigramJsonsFromDssmTextualFieldSetV1(bigrams);

    }
    void THitsSerializer::SetJsonsFromDssmTextualFieldSetWithoutTitle(TString words, TString bigrams) const {
        HitsBuf->SetWordJsonsFromDssmTextualFieldSetWithoutTitle(words);
        HitsBuf->SetBigramJsonsFromDssmTextualFieldSetWithoutTitle(bigrams);
    }

    void THitsSerializer::AddBlockHit(const TBlockHit& hit)
    {
        UpdateAuxDataInProto(hit.Position, *HitsBuf);
        if (OffroadHitWriter) {
            const TLastAnnotationInStream& ann = LastAnnotations.at(TLastAnnotationKey{hit.Position.StreamRef().Type, hit.Position.AnnotationRef().StreamIndex});
            Y_ASSERT(ann.BreakNumber == hit.Position.Annotation->BreakNumber);
            TTextMachineHit offroadHit;
            ConvertToOffroad(hit, offroadHit, ann.AnnotationIndex);
            OffroadHitWriter->WriteHit(offroadHit);

            if (HitsBuf->HitWeightsSize()) {
                HitsBuf->AddHitWeights(hit.Weight);
            } else if (hit.Weight != 0) {
                if (!HitsBuf->HitWeightsSize()) {
                    // lazy-initialize
                    for (size_t i = 0; i < HitCount; i++)
                        HitsBuf->AddHitWeights(0);
                }
                HitsBuf->AddHitWeights(hit.Weight);
            }

            HitCount++;
        } else {
            TPbBlockHit& hitProto = *HitsBuf->AddBlockHits();
            ConvertToProto(hit, hitProto);
        }
    }

    void THitsSerializer::Finish()
    {
        if (OffroadHitWriter) {
            OffroadHitWriter->Finish();
            HitsBuf->SetPackedBlockHitsCount(HitCount);
        }
        if (OffroadAnnotationWriter && !OffroadAnnotationWriter->IsFinished()) {
            OffroadAnnotationWriter->Finish();
            ::google::protobuf::RepeatedPtrField<NTextMachineProtocol::TPbStream> existingStreams;
            THashMap<EStreamType, ui32> stream2idx;
            for (size_t i = 0; i < HitsBuf->StreamsSize(); i++) {
                if (HitsBuf->GetStreams(i).HasAnnotationCount()) {
                    EStreamType type = static_cast<EStreamType>(HitsBuf->GetStreams(i).GetType());
                    existingStreams.Add()->Swap(HitsBuf->MutableStreams(i));
                    stream2idx.emplace(type, stream2idx.size());
                }
            }
            HitsBuf->MutableStreams()->Swap(&existingStreams);
            HitsBuf->SetPackedAnnotationCount(AnnotationStreamTypes.size());
            if (stream2idx.size() > 1) {
                size_t streamBits = MostSignificantBit(stream2idx.size() - 1) + 1;
                TStringOutput annotationStreamTypesOut(*HitsBuf->MutablePackedAnnotationStreams());
                NOffroad::TBitOutput annotationStreamTypesBitOut(&annotationStreamTypesOut);
                annotationStreamTypesBitOut.Write(streamBits, 6);
                for (EStreamType streamType : AnnotationStreamTypes)
                    annotationStreamTypesBitOut.Write(stream2idx.at(streamType), streamBits);
                annotationStreamTypesBitOut.Finish();
            }
        }
    }

    TStream& THitsAuxData::StreamRef(EStreamType streamType) {
        auto iter = Streams.find(streamType);
        if (iter != Streams.end()) {
            return iter->second;
        }
        auto& stream = Streams[streamType];
        stream.SetType(streamType);
        stream.AnnotationCount = 0;
        stream.WordCount = 0;
        stream.MaxValue = 0;
        return stream;
    }

    const TStream* THitsAuxData::StreamPtr(EStreamType streamType) const {
        return Streams.FindPtr(streamType);
    }

    THitsAuxData::TStreamAnnotations& THitsAuxData::AnnotationsRef(
        EStreamType streamType)
    {
        return Annotations[streamType];
    }

    const THitsAuxData::TStreamAnnotations* THitsAuxData::AnnotationsPtr(
        EStreamType streamType) const
    {
        return Annotations.FindPtr(streamType);
    }

    const TAnnotation& THitsAuxData::AdHocAnnotationRef(
        EStreamType streamType,
        size_t breakNum,
        size_t streamIndex)
    {
        AdHocAnnotations.emplace_back();

        TAnnotation& newAnn = AdHocAnnotations.back();
        newAnn.Stream = &StreamRef(streamType);
        newAnn.BreakNumber = breakNum;
        newAnn.FirstWordPos = 0;
        newAnn.Length = 0;
        newAnn.Value = 0.0;
        newAnn.Text = TString();
        newAnn.Language = LANG_UNK;
        newAnn.StreamIndex = streamIndex;

        return newAnn;
    }

    void THitsDeserializer::PrepareAuxDataFromProto(
        const TPbHits& src,
        THitsAuxData& dst,
        bool checkOrder)
    {
        for (size_t streamIndex = 0; streamIndex != src.StreamsSize(); ++streamIndex) {
            TErrorGuard guard1{Handler, "stream", streamIndex};

            const TPbStream& streamProto = src.GetStreams(streamIndex);
            size_t streamTypeIndex = streamProto.GetType();

            if (!TStream::HasIndex(streamTypeIndex)) {
                Handler.Error(TDeserializerError() << "bad stream type index, " << streamTypeIndex);
                continue;
            }

            TStream& stream = dst.StreamRef(static_cast<EStreamType>(streamTypeIndex));
            ConvertFromProto(streamProto, stream);
            auto& annotations = dst.AnnotationsRef(static_cast<EStreamType>(streamTypeIndex));
            annotations.resize(streamProto.AnnotationsSize());

            for(size_t annIndex = 0; annIndex != streamProto.AnnotationsSize(); ++annIndex) {
                TErrorGuard guard2{Handler, "annotation", annIndex};

                const TPbAnnotation& annProto = streamProto.GetAnnotations(annIndex);
                TAnnotation& ann = annotations[annIndex];
                ConvertFromProto(annProto, streamTypeIndex, dst, ann);

                if (checkOrder && annIndex > 0) {
                    const auto prevPair = std::make_pair(
                        annotations[annIndex - 1].BreakNumber,
                        annotations[annIndex - 1].StreamIndex);
                    const auto curPair = std::make_pair(ann.BreakNumber, ann.StreamIndex);

                    if (!(prevPair < curPair)) {
                        Handler.Error(TDeserializerError()
                            << "wrong order of annotations for stream " << stream.Type
                            << ", must be ordered by pair (break number, stream index)");
                    }
                }
            }
        }
    }

    void THitsDeserializer::LoadAnnotationsFromOffroad()
    {
        using TAnnStreamsReader = NOffroad::TFlatSearcher<std::nullptr_t, ui32, NOffroad::TNullVectorizer, NOffroad::TUi32Vectorizer>;
        THolder<TAnnStreamsReader> annStreamsReader;
        size_t annCount = HitsBuf->GetPackedAnnotationCount();
        OrderedAnnotations.resize(annCount, nullptr);
        TMap<EStreamType, size_t> processedAnnotationsCount;
        if (HitsBuf->GetPackedAnnotationStreams()) {
            annStreamsReader = MakeHolder<TAnnStreamsReader>(MakeArrayRef(HitsBuf->GetPackedAnnotationStreams()));
            // size reported by TAnnStreamsReader can be greater than the real one due to padding bits;
            // however, it cannot be smaller
            if (annCount > annStreamsReader->Size()) {
                Handler.Error(TDeserializerError() << "unexpected EOF in packed annotations");
                annCount = annStreamsReader->Size();
            }
        }
        THolder<TTextMachineAnnotationReader> reader = MakeHolder<TTextMachineAnnotationReader>();
        reader->Reset(HugeSingleton<TTextMachineAnnotationReaderTableV1>(), MakeArrayRef(HitsBuf->GetPackedAnnotations()));
        TTextMachineAnnotationHit hit;
        for (size_t annIndex = 0; annIndex < annCount; annIndex++) {
            if (!reader->ReadHit(&hit)) {
                Handler.Error(TDeserializerError() << "unexpected EOF in packed annotations");
                break;
            }
            ui32 streamId = annStreamsReader ? annStreamsReader->ReadData(annIndex) : 0;
            if (streamId >= HitsBuf->StreamsSize()) {
                Handler.Error(TDeserializerError() << "broken annotation info");
                continue;
            }
            size_t streamTypeIndex = HitsBuf->GetStreams(streamId).GetType();
            if (!TStream::HasIndex(streamTypeIndex))
                continue; // error already reported when parsing streams
            EStreamType streamType = static_cast<EStreamType>(streamTypeIndex);
            auto& annotations = AuxData->AnnotationsRef(streamType);
            size_t annInStream = processedAnnotationsCount[streamType]++;
            Y_ASSERT(annotations.size() >= annInStream);
            if (annotations.size() == annInStream) {
                annotations.emplace_back(TAnnotation::DontInitialize());
                annotations.back().Stream = AuxData->StreamPtr(streamType);
                annotations.back().Language = LANG_UNK;
            } // else Stream, Text and Language are already initialized
            ConvertFromOffroad(hit, annotations[annInStream]);
            OrderedAnnotations[annIndex] = &annotations[annInStream];
        }
    }

    void THitsDeserializer::ConvertFromProto(
        const NTextMachineProtocol::TPbStream& src,
        TStream& dst)
    {
        size_t streamTypeIndex = src.GetType();
        Y_ASSERT(TStream::HasIndex(streamTypeIndex));

        dst.SetType(static_cast<EStreamType>(streamTypeIndex));
        dst.AnnotationCount = src.GetAnnotationCount();
        dst.WordCount = src.GetWordCount();
        dst.MaxValue = src.GetMaxValue();
    }

    void THitsDeserializer::ConvertFromProto(
        const NTextMachineProtocol::TPbAnnotation& src,
        size_t streamTypeIndex,
        const THitsAuxData& data,
        TAnnotation& dst)
    {
        ELanguage lang = LANG_UNK;
        if (src.GetLanguage() >= LANG_MAX) {
            Handler.Error(TDeserializerError{} << "bad language index, " << src.GetLanguage());
            ythrow TBadHit{};
        } else {
            lang = static_cast<ELanguage>(src.GetLanguage());
        }

        Y_ASSERT(TStream::HasIndex(streamTypeIndex));

        const TStream* stream = data.StreamPtr(static_cast<EStreamType>(streamTypeIndex));
        Y_ASSERT(!!stream);

        dst.BreakNumber = src.GetBreakNumber();
        dst.FirstWordPos = src.GetFirstWordPos();
        dst.Length = src.GetLength();
        dst.Value = src.GetValue();
        dst.Stream = stream;
        dst.Text = src.GetText();
        dst.Language = lang;
        dst.StreamIndex = src.GetStreamIndex();
    }

    void THitsDeserializer::ConvertFromOffroad(const TTextMachineAnnotationHit& src, TAnnotation& dst)
    {
        dst.BreakNumber = src.BreakNumber;
        dst.FirstWordPos = src.FirstWordPos;
        dst.Length = src.Length;
        dst.Value = src.Weight;
        dst.StreamIndex = src.StreamIndex;
        // Stream, Text, Language are not initialized
    }

    void THitsDeserializer::ConvertFromProto(
        const NTextMachineProtocol::TPbPosition& src,
        THitsAuxData& data, TPosition& dst)
    {
        TErrorGuard guard{Handler, "position"};

        dst.LeftWordPos = src.GetLeftWordPos();
        dst.RightWordPos = src.GetRightWordPos();
        dst.Relevance = src.GetRelevance();
        const size_t streamTypeIndex = src.GetStreamType();

        if (!TStream::HasIndex(streamTypeIndex)) {
            Handler.Error(TDeserializerError() << "bad stream type index in hit, " << streamTypeIndex);
            ythrow TBadHit{};
        }
        const EStreamType streamType = static_cast<EStreamType>(streamTypeIndex);

        const size_t breakNum = src.GetBreakNumber();
        const size_t streamIndex = src.GetStreamIndex();

        const TAnnotation* ann = LookupAnnotation(
            data, streamType,
            breakNum, streamIndex);

        if (!ann) {
            Handler.Error(TDeserializerError{}
                << "hit refers to missing annotation "
                << breakNum << " in stream " << streamType
                << "(" << streamIndex << ")");

            dst.Annotation = &data.AdHocAnnotationRef(streamType, breakNum, streamIndex);
        } else {
            dst.Annotation = ann;
        }
    }

    void THitsDeserializer::ConvertFromProto(
        const NTextMachineProtocol::TPbWordHit& src,
        THitsAuxData& data,
        THit& hit)
    {
        hit.Word.QueryId = src.GetRequestIndex();
        hit.Word.WordId = src.GetWordIndex();
        hit.Word.FormId = src.GetFormIndex();
        ConvertFromProto(src.GetPosition(), data, hit.Position);
        hit.Weight = src.GetWeight();
    }

    void THitsDeserializer::ConvertFromProto(
        const NTextMachineProtocol::TPbBlockHit& src,
        THitsAuxData& data,
        TBlockHit& hit)
    {
        EBaseIndexLayerType baseIndexLayer = TBaseIndexLayer::Layer0;
        if (!TBaseIndexLayer::HasIndex(src.GetLayer())) {
            Handler.Error(TDeserializerError{} << "bad layer type index, " << src.GetLayer());
        } else {
            baseIndexLayer = static_cast<EBaseIndexLayerType>(src.GetLayer());
        }

        EMatchPrecisionType precision = TMatchPrecision::Lemma;
        if (!TMatchPrecision::HasIndex(src.GetPrecision())) {
            Handler.Error(TDeserializerError{} << "bad match precision index, " << src.GetPrecision());
        } else {
            precision = static_cast<EMatchPrecisionType>(src.GetPrecision());
        }

        hit.Layer = baseIndexLayer;
        hit.BlockId = src.GetBlockIndex();
        hit.Precision = precision;
        hit.LemmaId = src.GetLemmaIndex();
        hit.Weight = src.GetWeight();
        hit.FormId = src.GetFormIndex();
        hit.LowLevelFormId = src.GetLowLevelFormIndex();
        ConvertFromProto(src.GetPosition(), data, hit.Position);
    }

    void THitsDeserializer::ConvertFromOffroad(const TTextMachineHit& src, TBlockHit& dst)
    {
        dst.Layer = TBaseIndexLayer::Layer0;
        if (!TBaseIndexLayer::HasIndex(src.Layer)) {
            Handler.Error(TDeserializerError{} << "bad layer type index, " << src.Layer);
        } else {
            dst.Layer = static_cast<EBaseIndexLayerType>(src.Layer);
        }
        dst.Precision = TMatchPrecision::Lemma;
        if (!TMatchPrecision::HasIndex(src.Precision)) {
            Handler.Error(TDeserializerError{} << "bad match precision index, " << src.Precision);
        } else {
            dst.Precision = static_cast<EMatchPrecisionType>(src.Precision);
        }

        dst.Position.LeftWordPos = src.LeftWordPos;
        dst.Position.RightWordPos = src.LeftWordPos + src.RightWordPosDelta;
        dst.Position.Relevance = src.Relevance;
        dst.BlockId = src.BlockIndex;
        dst.LemmaId = src.LemmaIndex;
        dst.FormId = src.FormIndex;
        dst.LowLevelFormId = src.LowLevelFormIndex;
        // Annotation, Weight are not initialized
    }

    bool THitsDeserializer::NextWordHit(THit& hit)
    {
        Y_ENSURE_EX(AuxData, TDeserializerError{} << "deserializer must be initialized");

        TErrorGuard guard{Handler, "word_hit", CurWordHit};

        for (;;) {
            if (CurWordHit >= HitsBuf->WordHitsSize()) {
                return false;
            }

            try {
                ConvertFromProto(HitsBuf->GetWordHits(CurWordHit), *AuxData.Get(), hit);
                CurWordHit += 1;
            } catch (TBadHit&) {
                CurWordHit += 1;
                continue;
            } catch (...) {
                CurWordHit += 1;
                throw;
            }

            break;
        }

        return true;
    }

    bool THitsDeserializer::NextBlockHit(TBlockHit& hit)
    {
        Y_ENSURE_EX(AuxData, TDeserializerError{} << "deserializer must be initialized");

        TErrorGuard guard{Handler, "block_hit", CurBlockHit};

        size_t numBlockHits = OffroadHitReader ? HitsBuf->GetPackedBlockHitsCount() : HitsBuf->BlockHitsSize();

        for (;;) {
            if (CurBlockHit >= numBlockHits) {
                return false;
            }

            try {
                if (OffroadHitReader) {
                    TTextMachineHit offroadHit;
                    if (!OffroadHitReader->ReadHit(&offroadHit)) {
                        Handler.Error(TDeserializerError{} << "unexpected EOF in packed hits");
                        return false;
                    }
                    if (offroadHit.AnnotationIndex >= OrderedAnnotations.size()) {
                        Handler.Error(TDeserializerError{} << "broken hit info");
                        continue;
                    }
                    if (!OrderedAnnotations[offroadHit.AnnotationIndex]) {
                        // bad annotation already reported
                        continue;
                    }
                    hit.Position.Annotation = OrderedAnnotations[offroadHit.AnnotationIndex];
                    hit.Weight = (CurBlockHit >= HitsBuf->HitWeightsSize() ? 0 : HitsBuf->GetHitWeights(CurBlockHit));
                    ConvertFromOffroad(offroadHit, hit);
                } else {
                    ConvertFromProto(HitsBuf->GetBlockHits(CurBlockHit), *AuxData.Get(), hit);
                }
                CurBlockHit += 1;
            } catch (TBadHit&) {
                CurBlockHit += 1;
                continue;
            } catch (...) {
                CurBlockHit += 1;
                throw;
            }

            break;
        }

        return true;
    }

    TString THitsDeserializer::GetFullErrorMessage() const {
        return Handler.GetFullErrorMessage("<deserializer message> ");
    }

    void THitsDeserializer::Init() {
        AuxData.Reset(new THitsAuxData);
        if (HitsBuf->GetPackVersion() != 0 && HitsBuf->GetPackVersion() != 1) {
            Handler.Error(TDeserializerError{} << "unknown pack version, " << HitsBuf->GetPackVersion());
        }
        if (HitsBuf->GetPackVersion() == 1) {
            PrepareAuxDataFromProto(*HitsBuf, *AuxData.Get(), false);
            if (HitsBuf->GetPackedBlockHits().size() % 16 || HitsBuf->GetPackedAnnotations().size() % 16) {
                Handler.Error(TDeserializerError{} << "broken packed data");
                return;
            }
            LoadAnnotationsFromOffroad();
            OffroadHitReader = MakeHolder<TTextMachineHitReader>();
            OffroadHitReader->Reset(HugeSingleton<TTextMachineHitReaderTableV1>(), MakeArrayRef(HitsBuf->GetPackedBlockHits()));
        } else {
            PrepareAuxDataFromProto(*HitsBuf, *AuxData.Get(), true);
        }
    }

    THitsDeserializer::THitsDeserializer(
        const NTextMachineProtocol::TPbHits* hitsBuf,
        const TOptions& opts)
        : Opts(opts)
        , Handler(opts.FailMode)
        , HitsBuf(hitsBuf)
    {
        Y_ASSERT(HitsBuf);
    }

    THitsDeserializer::~THitsDeserializer() = default;
} // NTextMachine

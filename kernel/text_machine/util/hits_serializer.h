#pragma once

#include <kernel/text_machine/interface/hit.h>
#include <kernel/text_machine/proto/text_machine.pb.h>

#include <kernel/lingboost/error_handler.h>

#include <util/generic/deque.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>

#include <array>

struct TTextMachineHit;
struct TTextMachineHitReader;
struct TTextMachineHitWriter;
struct TTextMachineAnnotationHit;
struct TTextMachineAnnotationReader;
struct TTextMachineAnnotationWriter;

namespace NTextMachine {
    class TSerializerError : public yexception {};
    class THitsOrderError : public TSerializerError {};

    class THitsSerializer {
    public:
        struct TOptions {
            bool UseOffroadCompression = true;
            TOptions() {}
        };

        explicit THitsSerializer(NTextMachineProtocol::TPbHits* hitsBuf, TOptions options = {});
        ~THitsSerializer();

        // Hits should be ordered by break number for each stream
        // Will throw yexception, if ordering is wrong
        // Will throw yexception, if input hit structures are malformed (e.g. contain null pointers)
        void AddWordHit(const THit& hit);
        void AddBlockHit(const TBlockHit& hit);
        void Finish();

        void SetJsonsFromDssmTextualFieldSetV1(TString words, TString bigrams) const;
        void SetJsonsFromDssmTextualFieldSetWithoutTitle(TString words, TString bigrams) const;

    public:
        static void ConvertToProto(const TAnnotation& src, NTextMachineProtocol::TPbAnnotation& dst);
        static void ConvertToProto(const TStream& src, NTextMachineProtocol::TPbStream& dst);
        static void ConvertToProto(const TPosition& src, NTextMachineProtocol::TPbPosition& dst);
        static void ConvertToProto(const THit& src, NTextMachineProtocol::TPbWordHit& dst);
        static void ConvertToProto(const TBlockHit& src, NTextMachineProtocol::TPbBlockHit& dst);
        static void ConvertToOffroad(const TAnnotation& src, TTextMachineAnnotationHit& dst);
        static void ConvertToOffroad(const TBlockHit& src, TTextMachineHit& dst, ui32 annotationIndex);

        void UpdateAuxDataInProto(const TPosition& src, NTextMachineProtocol::TPbHits& dst);

    private:
        using TLastAnnotationKey = std::pair<EStreamType, ui32 /*streamIndex*/>;
        struct TLastAnnotationInStream {
            ui32 BreakNumber;
            ui32 AnnotationIndex;
        };
        NTextMachineProtocol::TPbHits* HitsBuf;
        THolder<TStringOutput> OffroadHitStream, OffroadAnnotationStream;
        THolder<TTextMachineHitWriter> OffroadHitWriter;
        THolder<TTextMachineAnnotationWriter> OffroadAnnotationWriter;
        TVector<EStreamType> AnnotationStreamTypes;
        THashMap<TLastAnnotationKey, TLastAnnotationInStream> LastAnnotations;
        size_t HitCount = 0;
    };

    class THitsAuxData {
    public:
        using TStreamAnnotations = TDeque<TAnnotation>;

        TStream& StreamRef(EStreamType streamType);
        TStreamAnnotations& AnnotationsRef(EStreamType streamType);
        const TAnnotation& AdHocAnnotationRef(
            EStreamType streamType,
            size_t breakNum,
            size_t streamIndex);

        const TStream* StreamPtr(EStreamType streamType) const;
        const TStreamAnnotations* AnnotationsPtr(EStreamType streamType) const;

    private:
        using TStreams = TMap<EStreamType, TStream>;
        using TAnnotations = TMap<EStreamType, TStreamAnnotations>;

        TStreams Streams;
        TAnnotations Annotations;
        TStreamAnnotations AdHocAnnotations;
    };

    class TDeserializerError : public yexception {};

    class THitsDeserializer {
    public:
        using EFailMode = NLingBoost::TErrorHandler::EFailMode;

        struct TOptions {
            EFailMode FailMode = EFailMode::ThrowOnError;

            TOptions() {}
        };

    private:
        TOptions Opts;
        NLingBoost::TErrorHandler Handler;

        const NTextMachineProtocol::TPbHits* HitsBuf;
        THolder<THitsAuxData> AuxData;
        size_t CurWordHit = 0;
        size_t CurBlockHit = 0;
        THolder<TTextMachineHitReader> OffroadHitReader;
        TVector<const TAnnotation*> OrderedAnnotations;

        void LoadAnnotationsFromOffroad();

    public:
        THitsDeserializer(
            const NTextMachineProtocol::TPbHits* hitsBuf,
            const TOptions& opts = {});

        ~THitsDeserializer();

        // Must be called before Next*Hit
        // Will throw yexception, if protobuf is malformed
        // May leave deserializer in inconsistent state
        void Init();

        // Will throw yexception, if protobuf is malformed
        // Output hit structure may be incomplete/inconsistent
        // Deserializer retains consistent state
        bool NextWordHit(THit& hit);
        bool NextBlockHit(TBlockHit& hit);

        THolder<THitsAuxData>& GetHitsAuxData() {
            return AuxData;
        }

        bool IsInErrorState() const {
            return Handler.IsInErrorState();
        }
        void ClearErrorState() {
            Handler.ClearErrorState();
        }
        TString GetFullErrorMessage() const;

    public:
        void PrepareAuxDataFromProto(
            const NTextMachineProtocol::TPbHits& src,
            THitsAuxData& dst,
            bool checkOrder);

        void ConvertFromProto(
            const NTextMachineProtocol::TPbStream& src,
            TStream& dst);
        void ConvertFromProto(
            const NTextMachineProtocol::TPbAnnotation& src,
            size_t streamTypeIndex,
            const THitsAuxData& data,
            TAnnotation& dst);
        void ConvertFromProto(
            const NTextMachineProtocol::TPbPosition& src,
            THitsAuxData& data,
            TPosition& dst);
        void ConvertFromProto(
            const NTextMachineProtocol::TPbWordHit& src,
            THitsAuxData& data,
            THit& hit);
        void ConvertFromProto(
            const NTextMachineProtocol::TPbBlockHit& src,
            THitsAuxData& data,
            TBlockHit& hit);
        // leaves dst.Stream, dst.Text, dst.Language as is
        static void ConvertFromOffroad(const TTextMachineAnnotationHit& src, TAnnotation& dst);
        // leaves dst.Annotation, dst.Weight as is
        void ConvertFromOffroad(const TTextMachineHit& src, TBlockHit& dst);
    };
} // NTextMachine

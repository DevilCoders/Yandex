#pragma once

#include "query.h"
#include "types.h"

#include <kernel/lingboost/constants.h>
#include <kernel/text_machine/interface/stream_constants.h>

#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NTextMachine {
    using NLingBoost::EBaseIndexType;
    using NLingBoost::TBaseIndex;

    using NLingBoost::EBaseIndexLayerType;
    using NLingBoost::TBaseIndexLayer;

    using NLingBoost::EStreamType;
}

namespace NTextMachine {
    struct TStream
        : public NLingBoost::TStream
    {
        EBaseIndexType IndexType;
        EStreamType Type;
        TBreakId AnnotationCount;
        TStreamWordId WordCount;
        float MaxValue;

        struct DontInitialize {};

        explicit TStream(DontInitialize) {}

        TStream(
            EStreamType type,
            TBreakId annCount,
            TStreamWordId wordCount,
            float maxValue)
        {
            SetType(type);
            AnnotationCount = annCount;
            WordCount = wordCount;
            MaxValue = maxValue;
        }
        explicit TStream(EStreamType type)
            : TStream(
                type,
                TAbsentValue::StreamBreakCount,
                TAbsentValue::StreamWordCount,
                TAbsentValue::StreamMaxValue)
        {}
        TStream()
            : TStream(TStream::StreamMax)
        {}

        void SetType(EStreamType type) {
            IndexType = (TStream::StreamMax == type
                ? TBaseIndex::BaseIndexMax
                : NLingBoost::GetBaseIndexByStream(type));
            Type = type;
        }
        void Clear() {
            AnnotationCount = TAbsentValue::StreamBreakCount;
            WordCount = TAbsentValue::StreamWordCount;
            MaxValue = TAbsentValue::StreamMaxValue;
        }
    };

    struct TAnnotation {
        const TStream* Stream;
        TStreamIndex StreamIndex; // allows to have many annotations sources inside one stream type, used in onotole3
        TBreakId BreakNumber;
        TStreamWordId FirstWordPos; // == Invalid<TStreamWordId>() for sentence-oriented streams
        TBreakWordId Length;
        float Value;

        // For experiments only
        TString Text;
        ELanguage Language;

        struct DontInitialize {};

        explicit TAnnotation(DontInitialize) {}

        TAnnotation(
            const TStream* streamPtr,
            ui16 streamIndex,
            TBreakId breakNumber,
            TStreamWordId firstWordPos,
            TBreakWordId breakLength,
            float value)
            : Stream(streamPtr)
            , StreamIndex(streamIndex)
            , BreakNumber(breakNumber)
            , FirstWordPos(firstWordPos)
            , Length(breakLength)
            , Value(value)
            , Language(LANG_UNK)
        {}
        TAnnotation(
            const TStream* streamPtr,
            ui16 streamIndex = TAbsentValue::StreamIndex)
            : TAnnotation(
                streamPtr,
                streamIndex,
                TAbsentValue::BreakId,
                TAbsentValue::StreamWordPos,
                TAbsentValue::BreakWordCount,
                TAbsentValue::AnnotationValue)
        {}
        TAnnotation()
            : TAnnotation(nullptr)
        {}

        const TStream& StreamRef() const {
            Y_ASSERT(Stream);
            return *Stream;
        }
        const TStream* StreamPtr() const {
            return Stream;
        }
    };

    struct TPosition {
        const TAnnotation* Annotation;
        TBreakWordId LeftWordPos;
        TBreakWordId RightWordPos;
        THitRelevance Relevance;

        struct DontInitialize {};

        explicit TPosition(DontInitialize) {}

        TPosition(
            const TAnnotation* annotation,
            TBreakWordId leftWordPos,
            TBreakWordId rightWordPos)
            : Annotation(annotation)
            , LeftWordPos(leftWordPos)
            , RightWordPos(rightWordPos)
            , Relevance(TAbsentValue::HitRelevance)
        {}
        TPosition(
            const TAnnotation* annotation)
            : TPosition(
                annotation,
                TAbsentValue::BreakWordPos,
                TAbsentValue::BreakWordPos)
        {}
        TPosition()
            : TPosition(nullptr)
        {}

        const TAnnotation& AnnotationRef() const {
            Y_ASSERT(Annotation);
            return *Annotation;
        }
        const TAnnotation* AnnotationPtr() const {
            return Annotation;
        }
        const TStream& StreamRef() const {
            return AnnotationRef().StreamRef();
        }
        const TStream* StreamPtr() const {
            return Annotation ? Annotation->StreamPtr() : nullptr;
        }
    };

    struct THitWord {
        TQueryId QueryId;
        TQueryWordId WordId;
        TWordFormId FormId;

        struct DontInitialize {};

        explicit THitWord(DontInitialize) {}

        THitWord(
            TQueryId queryId,
            TQueryWordId wordId,
            TWordFormId formId)
            : QueryId(queryId)
            , WordId(wordId)
            , FormId(formId)
        {}
        THitWord()
            : THitWord(
                TAbsentValue::QueryId,
                TAbsentValue::QueryWordId,
                TAbsentValue::WordFormId)
        {}
    };

    struct THit {
        TPosition Position;
        THitWord Word;
        float Weight;
        TWordFormId FormId;

        struct DontInitialize {};

        explicit THit(DontInitialize)
            : Position(TPosition::DontInitialize{})
            , Word(THitWord::DontInitialize{})
        {}

        THit(const TPosition& position,
            const THitWord& word,
            float weight)
            : Position(position)
            , Word(word)
            , Weight(weight)
            , FormId(TAbsentValue::HitFormId)
        {}
        THit(const TPosition& position,
            const THitWord& word,
            float weight,
            TWordFormId formId)
            : Position(position)
            , Word(word)
            , Weight(weight)
            , FormId(formId)
        {}
        THit()
            : THit(
                {},
                {},
                TAbsentValue::HitWeight)
        {}

        const TPosition& PositionRef() const {
            return Position;
        }
        const TAnnotation& AnnotationRef() const {
            return Position.AnnotationRef();
        }
        const TAnnotation* AnnotationPtr() const {
            return Position.AnnotationPtr();
        }
        const TStream& StreamRef() const {
            return Position.StreamRef();
        }
        const TStream* StreamPtr() const {
            return Position.StreamPtr();
        }
    };

    // Block hit from base search
    // Translated to one or more THits
    // by TWordHitsDispatcher
    struct TBlockHit {
        EBaseIndexLayerType Layer;
        TPosition Position;
        TBlockId BlockId;
        EMatchPrecisionType Precision;
        TWordLemmaId LemmaId;
        float Weight;
        TWordFormId FormId; // zero for forms not in qtree, 1+ for forms in qtree
        TLowLevelWordFormId LowLevelFormId;

        struct DontInitialize {};

        explicit TBlockHit(DontInitialize)
            : Position(TPosition::DontInitialize{})
        {}

        TBlockHit(
            EBaseIndexLayerType layer,
            const TPosition& position,
            TBlockId blockId,
            EMatchPrecisionType precision,
            TWordLemmaId lemmaId,
            float weight,
            TWordFormId formId,
            TLowLevelWordFormId lowLevelFormId)
            : Layer(layer)
            , Position(position)
            , BlockId(blockId)
            , Precision(precision)
            , LemmaId(lemmaId)
            , Weight(weight)
            , FormId(formId)
            , LowLevelFormId(lowLevelFormId)
        {}
        TBlockHit()
            : TBlockHit(
                TBaseIndexLayer::BaseIndexLayerMax,
                {},
                TAbsentValue::BlockId,
                TMatchPrecision::MatchPrecisionMax,
                TAbsentValue::HitLemmaId,
                TAbsentValue::HitWeight,
                TAbsentValue::HitFormId,
                TAbsentValue::HitLowLevelFormId)
        {}

        const TPosition& PositionRef() const {
            return Position;
        }
        const TAnnotation& AnnotationRef() const {
            return Position.AnnotationRef();
        }
        const TAnnotation* AnnotationPtr() const {
            return Position.AnnotationPtr();
        }
        const TStream& StreamRef() const {
            return Position.StreamRef();
        }
        const TStream* StreamPtr() const {
            return Position.StreamPtr();
        }
    };

    // Block hit with corresponding
    // array of word hits
    struct TMultiHit {
        TBlockHit BlockHit;
        const THitWord* Hits;
        size_t Count;

        struct DontInitialize {};

        explicit TMultiHit(DontInitialize)
            : BlockHit(TBlockHit::DontInitialize{})
        {}

        TMultiHit(
            const TBlockHit& blockHit,
            const THitWord* hits,
            size_t count)
            : BlockHit(blockHit)
            , Hits(hits)
            , Count(count)
        {}
        TMultiHit()
            : TMultiHit(
                {},
                nullptr,
                0)
        {}

        const TPosition& PositionRef() const {
            return BlockHit.PositionRef();
        }
        const TAnnotation& AnnotationRef() const {
            return BlockHit.AnnotationRef();
        }
        const TAnnotation* AnnotationPtr() const {
            return BlockHit.AnnotationPtr();
        }
        const TStream& StreamRef() const {
            return BlockHit.StreamRef();
        }
        const TStream* StreamPtr() const {
            return BlockHit.StreamPtr();
        }
    };
} //NTextMachine

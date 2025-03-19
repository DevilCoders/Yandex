#pragma once
#include <library/cpp/offroad/codec/encoder_64.h>
#include <library/cpp/offroad/tuple/tuple_reader.h>
#include <library/cpp/offroad/tuple/tuple_writer.h>
#include <library/cpp/packedtypes/zigzag.h>
#include <library/cpp/resource/resource.h>

#include <util/generic/bitops.h>
#include <util/generic/cast.h>

struct TTextMachineHit
{
    ui32 AnnotationIndex = 0;
    ui32 LeftWordPos = 0;
    ui32 RightWordPosDelta = 0;
    ui32 Relevance = 0;
    ui32 BlockIndex = 0;
    ui32 Precision = 0;
    ui32 LemmaIndex = 0;
    ui32 FormIndex = 0;
    ui32 LowLevelFormIndex = 0;
    ui32 Layer = 0;
};

struct TTextMachineHitVectorizer
{
    enum {
        TupleSize = 10,
    };

    template<class Slice>
    static void Scatter(const TTextMachineHit& hit, Slice&& slice) {
        slice[0] = hit.AnnotationIndex;
        slice[1] = hit.LeftWordPos;
        slice[2] = hit.RightWordPosDelta;
        slice[3] = hit.Relevance;
        slice[4] = hit.BlockIndex;
        slice[5] = hit.Precision;
        slice[6] = hit.LemmaIndex;
        slice[7] = hit.FormIndex;
        slice[8] = hit.LowLevelFormIndex;
        slice[9] = hit.Layer;
    }

    template<class Slice>
    static void Gather(Slice&& slice, TTextMachineHit* hit) {
        hit->AnnotationIndex = slice[0];
        hit->LeftWordPos = slice[1];
        hit->RightWordPosDelta = slice[2];
        hit->Relevance = slice[3];
        hit->BlockIndex = slice[4];
        hit->Precision = slice[5];
        hit->LemmaIndex = slice[6];
        hit->FormIndex = slice[7];
        hit->LowLevelFormIndex = slice[8];
        hit->Layer = slice[9];
    }
};

struct TTextMachineHitSubtractor
{
    enum {
        TupleSize = 10,
        PrefixSize = 0,
    };

    template<class Storage>
    static void Integrate(Storage&&) {
    }

    template<class Value, class Delta, class Next>
    static void Integrate(Value&& value, Delta&& delta, Next&& next) {
        next[0] = value[0] + ZigZagDecode(delta[0]);
        for (int i = 1; i < TupleSize; i++)
            next[i] = delta[i] ? delta[i] - 1 : value[i];
    }

    template<class Value, class Delta, class Next>
    static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
        delta[0] = ZigZagEncode<i32>(next[0] - value[0]);
        for (int i = 1; i < TupleSize; i++)
            delta[i] = next[i] == value[i] ? 0 : next[i] + 1;
    }
};

// inheritance instead of "using" to make forward declarations work
struct TTextMachineHitReader : public NOffroad::TTupleReader<TTextMachineHit, TTextMachineHitVectorizer, TTextMachineHitSubtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer> {
};
struct TTextMachineHitWriter : public NOffroad::TTupleWriter<TTextMachineHit, TTextMachineHitVectorizer, TTextMachineHitSubtractor, NOffroad::TEncoder64, 1, NOffroad::PlainOldBuffer> {
};

struct TTextMachineAnnotationHit
{
    ui32 BreakNumber = 0;
    ui32 StreamIndex = 0;
    ui32 FirstWordPos = Max<ui32>();
    ui32 Length = 0;
    float Weight = 1.0f;
};

struct TTextMachineAnnotationHitVectorizer
{
    enum {
        TupleSize = 5,
    };

    static const ui32 One = 0x3F800000; // 1.0f as ui32

    template<class Slice>
    static void Scatter(const TTextMachineAnnotationHit& hit, Slice&& slice) {
        slice[0] = hit.BreakNumber;
        slice[1] = hit.StreamIndex;
        slice[2] = hit.FirstWordPos + 1;
        slice[3] = hit.Length;
        slice[4] = One - BitCast<ui32>(hit.Weight); // turn 1.0f into zero
    }

    template<class Slice>
    static void Gather(Slice&& slice, TTextMachineAnnotationHit* hit) {
        hit->BreakNumber = slice[0];
        hit->StreamIndex = slice[1];
        hit->FirstWordPos = slice[2] - 1;
        hit->Length = slice[3];
        hit->Weight = BitCast<float>(One - slice[4]);
    }
};

struct TTextMachineAnnotationHitSubtractor
{
    enum {
        TupleSize = 5,
        PrefixSize = 0,
    };

    template<class Storage>
    static void Integrate(Storage&&) {
    }

    template<class Value, class Delta, class Next>
    static void Integrate(Value&& value, Delta&& delta, Next&& next) {
        next[0] = value[0] + ZigZagDecode(delta[0]);
        next[1] = delta[1];
        next[2] = value[2] + ZigZagDecode(delta[2]);
        next[3] = delta[3];
        next[4] = delta[4];
    }

    template<class Value, class Delta, class Next>
    static void Differentiate(Value&& value, Next&& next, Delta&& delta) {
        delta[0] = ZigZagEncode<i32>(next[0] - value[0]);
        delta[1] = next[1];
        delta[2] = ZigZagEncode<i32>(next[2] - value[2]);
        delta[3] = next[3];
        delta[4] = next[4];
    }
};

struct TTextMachineAnnotationReader : public NOffroad::TTupleReader<TTextMachineAnnotationHit, TTextMachineAnnotationHitVectorizer, TTextMachineAnnotationHitSubtractor, NOffroad::TDecoder64, 1, NOffroad::PlainOldBuffer> {
};
struct TTextMachineAnnotationWriter : public NOffroad::TTupleWriter<TTextMachineAnnotationHit, TTextMachineAnnotationHitVectorizer, TTextMachineAnnotationHitSubtractor, NOffroad::TEncoder64, 1, NOffroad::PlainOldBuffer> {
};

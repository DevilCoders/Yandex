#include "title_ranges.h"

#include <library/cpp/resource/resource.h>

#include <library/cpp/charset/recyr.hh>
#include <util/charset/wide.h>
#include <util/generic/bitmap.h>
#include <util/generic/hash.h>
#include <util/generic/typetraits.h>
#include <util/generic/algorithm.h>
#include <util/ysaveload.h>
#include <library/cpp/bit_io/bitoutput.h>
#include <library/cpp/bit_io/bitinput.h>
#include <util/stream/output.h>
#include <util/generic/ymath.h>
#include <util/generic/bitops.h>

template<>
void Out<TDocTitleRange>(IOutputStream& out, TTypeTraits<TDocTitleRange>::TFuncParam range) {
    out << range.ToString();
}

/////////////////////////////////////////

template <
    class TValue,
    ui8 Width,
    ui64 MinVal = ui64(0),
    ui64 MaxVal = (ui64(1) << Width) - ui64(1)>
struct TIntegralTypeBitSerializer {

    static_assert(std::is_integral<TValue>::value || std::is_enum<TValue>::value, "");
    static_assert(!std::is_signed<TValue>::value, "");
    static_assert(Width < 64, "");

    template <class TBitOutput>
    static void SaveBits(TBitOutput& out, TValue value)
    {
        const ui64 buffer = (ui64) value;
        CheckValue(buffer);
        out.Write(buffer, Width);
    }

    template <class TBitInput>
    static void LoadBits(TBitInput& in, TValue& value)
    {
        ui64 buffer;
        in.ReadSafe(buffer, Width);
        CheckValue(buffer);
        value = (TValue) buffer;
    }

private:
    static inline void CheckValue(ui64 value)
    {
        Y_ENSURE(ui64(0) == (InverseMaskLowerBits(Width) & value),
            "value " << value << " doesn't fit into " << ui64(Width) << " bits");

        Y_ENSURE(value <= MaxVal && value >= MinVal,
            "value " << value << " is out of range [" << MinVal << ";" << MaxVal << "]");
    }
};

template < class TRange, class TTraits >
struct TSimpleRangeBitSerializer {

    typedef typename TRange::TAtom TAtom;
    typedef TIntegralTypeBitSerializer<ui64, TTraits::RangeStateBits, 0, TRange::RangeNumStates - 1> TStateSerializer;
    typedef TIntegralTypeBitSerializer<TAtom, TTraits::RangeEndBits> TEndSerializer;
    typedef TIntegralTypeBitSerializer<TAtom, TTraits::RangeBeginBits> TBeginSerializer;

public:
    template <class TBitOutput>
    static void SaveBits(TBitOutput& out, const TRange& range)
    {
        TEndSerializer::SaveBits(out, range.End);
        TBeginSerializer::SaveBits(out, range.Begin);
        TStateSerializer::SaveBits(out, (ui64) range.State);
    }

    template <class TBitInput>
    static void LoadBits(TBitInput& in, TRange& range)
    {
        TEndSerializer::LoadBits(in, range.End);
        TBeginSerializer::LoadBits(in, range.Begin);

        ui64 stateIndex;
        TStateSerializer::LoadBits(in, stateIndex);
        range.State = (typename TRange::RangeState) stateIndex;
    }
};

struct TDocTitleRangeBitSerializerTraits {
    enum {
        RangeEndBits = 10,
        RangeBeginBits = 10,
        RangeStateBits = 2
    };
};

typedef TSimpleRangeBitSerializer<TDocTitleRange, TDocTitleRangeBitSerializerTraits> TDocTitleRangeBitSerializer;
typedef TIntegralTypeBitSerializer<EDocTitleRangeType, 8, 0, DRT_NUM_ELEMENTS - 1> TDocTitleRangeTypeBitSerializer;

/////////////////////////////////////////

TDocTitleRangeCode SerializeDocTitleRangeAndType(const TDocTitleRange& range, EDocTitleRangeType rangeType)
{
    ui32 buffer = 0;
    NBitIO::TBitOutputArray out((char*) &buffer, sizeof(buffer));
    TDocTitleRangeTypeBitSerializer::SaveBits(out, rangeType);
    TDocTitleRangeBitSerializer::SaveBits(out, range);
    out.Flush();

    return buffer;
}

void DeserializeDocTitleRangeAndType(TDocTitleRangeCode value, TDocTitleRange& range, EDocTitleRangeType& rangeType)
{
    NBitIO::TBitInput in((char*) &value, (char*) &value + sizeof(value));
    TDocTitleRangeTypeBitSerializer::LoadBits(in, rangeType);
    TDocTitleRangeBitSerializer::LoadBits(in, range);
}

/////////////////////////////////////////

float GetTitleRangesMatchingScore(const TDocTitleRanges& queryRanges, const TDocTitleRanges& titleRanges)
{
    // Think of queryRanges as conjunctive query (empty range ~ True).
    // To get positive score queryRanges and titleRanges should be compatible.
    // They are incompatible if corresponding ranges are not empty and have empty intersection.
    //
    ui32 commonRangesNum = 0;
    ui32 conflictingRangesNum = 0;
    ui32 queryRangesNum = 0;
    ui32 titleRangesNum = 0;

    for (size_t i = 0; i != DRT_NUM_ELEMENTS; ++i) {
        if (!queryRanges[i].Empty()) {
            queryRangesNum += 1;
        }

        if (!titleRanges[i].Empty()) {
            titleRangesNum += 1;
        }

        if (!queryRanges[i].Empty() && !titleRanges[i].Empty()) {
            if (queryRanges[i].HasIntersection(titleRanges[i])) {
                commonRangesNum += 1;
            }
            else {
                conflictingRangesNum += 1;
            }
        }
    }

    if (queryRangesNum == 0 || titleRangesNum == 0) {
        return 0.0;
    }

    if (conflictingRangesNum > 0) {
        return 0.0;
    }

    // Cerr << "-D- " << commonRangesNum << " " << queryRangesNum << " " << titleRangesNum << Endl;

    float queryScore = (float) commonRangesNum / (float) queryRangesNum;
    float titleScore = (float) commonRangesNum / (float) titleRangesNum;

    return std::max(0.0, (ceil(10.0 * queryScore) - (1.0 - titleScore)) / 10.0);
}

float GetTitleRangesClassScore(const TDocTitleRanges& queryRanges)
{
    for (size_t i = 0; i != DRT_NUM_ELEMENTS; ++i) {
        if (!queryRanges[i].Empty()) {
            return 1.0;
        }
    }

    return 0.0;
}

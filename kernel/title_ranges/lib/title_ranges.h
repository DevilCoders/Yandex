#pragma once

#include "doc_title_range.h"

#include <util/string/cast.h>

#include <array>

template <class T>
class TSimpleRange {
    public:
        typedef T TAtom;

        TSimpleRange()
            : Begin()
            , End()
            , State(RangeEmpty)
        {}

        TSimpleRange(const T& begin, const T& end)
            : Begin(begin)
            , End(end)
            , State(begin <= end ? RangeNormal : RangeEmpty)
        {}

        void Clear() {
            State = RangeEmpty;
        }

        void SetAll() {
            State = RangeAll;
        }

        bool Empty() const {
            return State == RangeEmpty;
        }

        bool HasIntersection(const TSimpleRange<T>& range) const {
            return !GetIntersection(range).Empty();
        }

        void Add(const T& x) {
            switch (State) {
                case RangeEmpty:
                    Begin = x;
                    End = x;
                    State = RangeNormal;
                    break;
                case RangeAll:
                    break;
                case RangeNormal:
                    if (x < Begin) {
                        Begin = x;
                    } else if (x > End) {
                        End = x;
                    }
                    break;
                default: {
                    Y_ASSERT(false);
                    break;
                }
            }
        }

        void Add(const TSimpleRange<T>& range) {
            switch (range.State) {
                case RangeEmpty:
                    break;
                case RangeAll:
                    State = RangeAll;
                    break;
                case RangeNormal:
                    Add(range.Begin);
                    Add(range.End);
                    break;
                default: {
                    Y_ASSERT(false);
                    break;
                }
            }
        }

        TSimpleRange<T> GetIntersection(const TSimpleRange<T>& range) const {
            switch (State) {
                case RangeEmpty:
                    return TSimpleRange<T>();
                case RangeAll:
                    return range;
                case RangeNormal:
                    switch (range.State) {
                        case RangeEmpty:
                            return TSimpleRange<T>();
                        case RangeAll:
                            return *this;
                        case RangeNormal:
                            return TSimpleRange<T>(Max(Begin, range.Begin),
                                                   Min(End, range.End));
                        default: {
                            Y_ASSERT(false);
                            return TSimpleRange<T>();
                        }
                    }
                default: {
                    Y_ASSERT(false);
                    return TSimpleRange<T>();
                }
            }
        }

        // Normal * All ~ intersection
        // other cases ~ union
        //
        void Update(const TSimpleRange<T>& range) {
            if (State == RangeAll && range.State == RangeNormal) {
                *this = range;
            }
            else if (State == RangeNormal && range.State == RangeAll) {
                // do nothing
            }
            else {
                Add(range);
            }
        }

        TString ToString() const {
            switch (State) {
                case RangeEmpty:
                    return "none";
                case RangeAll:
                    return "all";
                case RangeNormal:
                    if (Begin == End) {
                        return ::ToString(Begin);
                    } else {
                        return ::ToString(Begin) + "-" + ::ToString(End);
                    }
                default: {
                    Y_ASSERT(false);
                    return "unknown";
                }
            }
        }

        template<class Q>
        bool operator == (const TSimpleRange<Q>& other) const {
            return State == other.State && Begin == other.Begin && End == other.End;
        }

        T Begin;
        T End;

        enum RangeState {
            RangeEmpty = 0,
            RangeAll = 1,
            RangeNormal = 2,

            RangeNumStates
        } State;
};

/////////////////////////////////////////

typedef TSimpleRange<ui16> TDocTitleRange;
typedef std::array<TDocTitleRange, DRT_NUM_ELEMENTS> TDocTitleRanges;

Y_DECLARE_PODTYPE(TDocTitleRange);

template<>
void Out<TDocTitleRange>(IOutputStream& out, TTypeTraits<TDocTitleRange>::TFuncParam range);

/////////////////////////////////////////

typedef ui32 TDocTitleRangeCode;

TDocTitleRangeCode SerializeDocTitleRangeAndType(const TDocTitleRange& range, EDocTitleRangeType rangeType);

void DeserializeDocTitleRangeAndType(TDocTitleRangeCode value, TDocTitleRange& range, EDocTitleRangeType& rangeType);

/////////////////////////////////////////

float GetTitleRangesMatchingScore(const TDocTitleRanges& queryRanges, const TDocTitleRanges& titleRanges);
float GetTitleRangesClassScore(const TDocTitleRanges& queryRanges);

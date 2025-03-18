#pragma once

#include <util/system/yassert.h>
#include <util/generic/ylimits.h>

#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NOffroad {
    /**
     * Key data that also stores the total number of hits in each record.
     */
    struct THitCountKeyData {
        THitCountKeyData() = default;
        THitCountKeyData(TDataOffset endOffset, size_t hitCount)
            : EndOffset(endOffset)
            , HitCount(hitCount)
        {
        }

        friend bool operator==(const THitCountKeyData& l, const THitCountKeyData& r) {
            return l.EndOffset == r.EndOffset && l.HitCount == r.HitCount;
        }

        friend bool operator!=(const THitCountKeyData& l, const THitCountKeyData& r) {
            return !(l == r);
        }

        TDataOffset EndOffset;
        size_t HitCount = 0;
    };

    class THitCountKeyDataVectorizer: private TDataOffsetVectorizer {
        using TBase = TDataOffsetVectorizer;

    public:
        enum {
            TupleSize = 3
        };

        template <class Slice>
        static void Scatter(const THitCountKeyData& data, Slice&& slice) {
            Y_ASSERT(data.HitCount < Max<ui32>());

            TBase::Scatter(data.EndOffset, slice);
            slice[2] = data.HitCount;
        }

        template <class Slice>
        static void Gather(Slice&& slice, THitCountKeyData* data) {
            TBase::Gather(slice, &data->EndOffset);
            data->HitCount = slice[2];
        }
    };

    using THitCountKeyDataSubtractor = TD2I1Subtractor;

    class THitCountKeyDataSerializer: private TDataOffsetSerializer {
        using TBase = TDataOffsetSerializer;

    public:
        enum {
            MaxSize = TBase::MaxSize
        };

        static size_t Serialize(const THitCountKeyData& data, ui8* output) {
            return TBase::Serialize(data.EndOffset, output);
        }

        static size_t Deserialize(const ui8* input, THitCountKeyData* data) {
            data->HitCount = 0;
            return TBase::Deserialize(input, &data->EndOffset);
        }
    };

    class THitCountKeyDataFactory {
    public:
        template <class Hit>
        static void AddHit(const Hit&, THitCountKeyData* data) {
            data->HitCount++;
        }

        static void SetEnd(const TDataOffset& end, THitCountKeyData* data) {
            data->EndOffset = end;
        }

        static TDataOffset End(const THitCountKeyData& data) {
            return data.EndOffset;
        }
    };

    inline THitCountKeyData operator+(const THitCountKeyData& s1, const THitCountKeyData& s2) {
        return THitCountKeyData(s1.EndOffset, s1.HitCount + s2.HitCount);
    }

}

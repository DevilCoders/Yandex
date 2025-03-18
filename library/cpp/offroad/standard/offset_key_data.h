#pragma once

#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/custom/subtractors.h>

namespace NOffroad {
    using TOffsetKeyData = TDataOffset;

    using TOffsetKeyDataVectorizer = TDataOffsetVectorizer;

    using TOffsetKeyDataSubtractor = TD2Subtractor;

    using TOffsetKeyDataSerializer = TDataOffsetSerializer;

    /**
     * Factory for offset key data.
     */
    class TOffsetKeyDataFactory {
    public:
        template <class Hit>
        static void AddHit(const Hit&, TOffsetKeyData*) {
        }

        static void SetEnd(const TDataOffset& end, TOffsetKeyData* data) {
            *data = end;
        }

        static TDataOffset End(const TOffsetKeyData& data) {
            return data;
        }
    };

}

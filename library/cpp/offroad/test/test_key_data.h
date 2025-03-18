#pragma once

#include <tuple>

#include <util/string/builder.h>
#include <util/stream/format.h>
#include <util/system/yassert.h>

#include <library/cpp/offroad/offset/data_offset.h>
#include <library/cpp/offroad/standard/offset_key_data.h>
#include <library/cpp/offroad/standard/hit_count_key_data.h>

namespace NOffroad {
    using TTestKeyData = TOffsetKeyData;
    using TTestKeyDataVectorizer = TOffsetKeyDataVectorizer;
    using TTestKeyDataSerializer = TOffsetKeyDataSerializer;
    using TTestKeyDataSubtractor = TOffsetKeyDataSubtractor;
    using TTestKeyDataFactory = TOffsetKeyDataFactory;

    TMap<TString, TTestKeyData> MakeTestKeyData(size_t count, TString suffix) {
        ui64 data = 1;

        TMap<TString, TTestKeyData> result;
        for (size_t i = 0; i < 7 * count; i += 7) {
            result[TStringBuilder() << LeftPad(i, 10, '0') << suffix] = TDataOffset::FromEncoded(data);
            data++;
        }

        return result;
    }

    TMap<TString, THitCountKeyData> MakeHitCountKeyData(size_t count, TString suffix) {
        TMap<TString, THitCountKeyData> result;

        size_t hitCount = 245657;
        for (const auto& pair : MakeTestKeyData(count, suffix)) {
            hitCount = (hitCount * 5591) % 7841;
            result[pair.first] = THitCountKeyData(pair.second, hitCount);
        }

        return result;
    }

}

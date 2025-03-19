#pragma once

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NPrsLog {
    template<typename T>
    TString CompressFeaturesIds(const TVector<T>& ids) {
        TVector<ui16> compressed(Reserve(ids.size()));
        for (const T id : ids) {
            compressed.push_back(static_cast<ui16>(id));
        }
        return TString(reinterpret_cast<const char*>(compressed.data()), 2 * compressed.size());
    }

    template<typename T>
    TVector<T> DecompressFeaturesIds(const TString& raw) {
        Y_ENSURE(raw.size() % 2 == 0);
        TVector<T> decompressed(Reserve(raw.size() / 2));

        for (size_t shift = 0; shift < raw.size(); shift += 2) {
            const ui16 id = *reinterpret_cast<const ui16*>(raw.data() + shift);
            decompressed.push_back(static_cast<T>(id));
        }
        return decompressed;
    }
} // NPrsLog

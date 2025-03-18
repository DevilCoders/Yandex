#pragma once

#include <util/string/hex.h> // String2Byte of HexDecode

namespace NStrm::NPackager {
    // cast uuid like "9a04f079-9840-4286-ab92-e65be0885f95" to 16 bytes
    static inline TString Uuid2Bytestring(const TString& uuid) {
        constexpr size_t uuidSize = 36;
        constexpr size_t resSize = 16;
        Y_ENSURE(uuid.Size() == uuidSize);

        TString res;
        res.resize(resSize);
        size_t j = 0;
        for (size_t i = 0; i < uuid.Size(); ++i) {
            if (uuid[i] == '-') {
                continue;
            }

            Y_ENSURE(i + 1 < uuidSize);
            Y_ENSURE(j < resSize);

            // String2Byte take 2 chars
            res[j] = String2Byte(&uuid[i]);
            ++i;
            ++j;
        }
        Y_ENSURE(j == resSize);
        return res;
    }

}

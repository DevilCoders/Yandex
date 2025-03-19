#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <library/cpp/string_utils/ascii_encode/ascii_encode.h>

#include "remap_table.h"

TString Encode(const TRemapTable& rt) {
    TString out;
    const TVector<float>& originalCP = rt.GetOriginalCP();
    EncodeAscii(reinterpret_cast<const char*>(originalCP.begin()), originalCP.size() * sizeof(float), &out);
    return out;
}

bool Decode(TRemapTable* pRes, const TString& sz) {
    TVector<unsigned char> buf;
    if (!DecodeAscii(sz, &buf))
        return false;

    const size_t elemSize = sizeof(float);
    size_t cpCount = buf.size() / elemSize;
    if (cpCount < 2) {
        return false;
    }

    *pRes = TRemapTable(reinterpret_cast<const float*>(buf.begin()), cpCount);
    return true;
}

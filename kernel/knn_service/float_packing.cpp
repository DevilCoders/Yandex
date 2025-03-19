#include "float_packing.h"

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <util/generic/yexception.h>
#include <util/generic/vector.h>
#include <util/generic/xrange.h>
#include <cmath>

static void Unpack16ToFloatInterleaved(const i16 * l, const i16 * r, float* ldst, float* rdst, int length) {
    while (length >= 4) {
        ldst[0] = l[0];
        ldst[1] = l[1];
        ldst[2] = l[2];
        ldst[3] = l[3];

        rdst[0] = r[0];
        rdst[1] = r[1];
        rdst[2] = r[2];
        rdst[3] = r[3];

        ldst += 4;
        rdst += 4;
        l += 4;
        r += 4;
        length -= 4;
    }

    while (length > 0) {
        ldst[0] = l[0];

        rdst[0] = r[0];

        ldst += 1;
        rdst += 1;
        l += 1;
        r += 1;
        length -= 1;
    }
}

float NKnnService::TUnpackable16DotProduct::DotProductUnpackable16Alloca(const i16 * l, const i16 * r, int length)
{
    float* leftBuf = (float*) alloca(length * sizeof(float));
    float* rightBuf = (float*) alloca(length * sizeof(float));

    Unpack16ToFloatInterleaved(l, r, leftBuf, rightBuf, length);

    return FloatPacking::UnPackSq<i16>(DotProduct(leftBuf, rightBuf, length));
}

float NKnnService::TUnpackable16DotProduct::DotProductUnpackable16Heap(const i16 * l, const i16 * r, int length)
{
    TVector<float> leftBuf(length);
    TVector<float> rightBuf(length);
    Unpack16ToFloatInterleaved(l, r, leftBuf.begin(), rightBuf.begin(), length);
    return FloatPacking::UnPackSq<i16>(DotProduct(leftBuf.begin(), rightBuf.begin(), length));
}

TString NKnnService::PackEmbedsAsInt8Base64(TArrayRef<const float> emb) {
    TVector<char> buf(emb.size());
    for (auto i : xrange(emb.size())) {
        buf[i] = FloatPacking::Pack<i8>(emb[i]);
    }
    return Base64EncodeUrl(TStringBuf(buf.begin(), buf.size()));
}

TVector<float> NKnnService::UnpackEmbedsAsInt8Base64(TStringBuf packed) {
    TString buf = Base64Decode(packed);
    TVector<float> emb(buf.size());
    for (auto i : xrange(emb.size())) {
        emb[i] = FloatPacking::UnPack<i8>(buf[i]);
    }
    return emb;
}

TString NKnnService::PackEmbedsBase64(TArrayRef<const float> emb) {
    return Base64EncodeUrl(TStringBuf((const char*) emb.begin(), (const char*) +emb.end()));
}

TVector<float> NKnnService::UnpackEmbedsBase64(TStringBuf packed) {
    TVector<float> res;
    TString buf = Base64Decode(packed);
    Y_ENSURE(buf.size() % sizeof(float) == 0, "can't decode float32 vector from data");
    TArrayRef<const float> reg((const float*)buf.begin(), (const float*) buf.end());
    res.assign(reg.begin(), reg.end());
    return res;
}

i64 NKnnService::DistanceToInteger(float dist) {
    //NOTE: right now we don't work outside [-2, 2] segment
    Y_ENSURE(std::abs(ClampVal<float>(dist, -2, 2) - dist) < 1e-5, "can't pack float: " << dist);

    i64 res = dist * 1e9;
    Y_ASSERT(std::abs(IntegerToDistance(res) - dist) < (1e-3 + 1e-3 * std::abs(dist)));
    return res;
}

float NKnnService::IntegerToDistance(i64 dist) {
    return float(dist) / 1e9;
}

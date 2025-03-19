#include "qd_saas_kv_key.h"

#include <util/digest/city.h>
#include <util/digest/numeric.h>
#include <util/string/cast.h>
#include <util/string/hex.h>
#include <util/stream/output.h>
#include <util/system/byteorder.h>

namespace NQueryDataSaaS {
    const size_t HASH128_HEX_SIZE = 2 * sizeof(TSaaSKVKey::THash128);

    namespace {
        inline TSaaSKVKey::THash128 FixKeyEndiannes(TSaaSKVKey::THash128 k) {
#if defined(_big_endian_)
            return TSaaSKey(LittleToBig(k.first), LittleToBig(k.second));
#else
            return k;
#endif
        }

        TSaaSKVKey DoCombineKeys(TSaaSKVKey a, TSaaSKVKey b) {
            return TSaaSKVKey{CombineHashes(a.GetHash128().second, b.GetHash128().first), CombineHashes(a.GetHash128().first, b.GetHash128().second)};
        }
    }

    TString TSaaSKVKey::AsString() const {
        TString s;
        AsString(s);
        return s;
    }

    void TSaaSKVKey::AsString(TString& s) const {
        char buf[HASH128_HEX_SIZE];
        auto rawKey = FixKeyEndiannes(Hash128);
        HexEncode(&rawKey, sizeof(rawKey), buf);
        s.append(buf, sizeof(buf));
    }

    TSaaSKVKey& TSaaSKVKey::Append(TSaaSKVKey o) {
        *this = DoCombineKeys(*this, o);
        return *this;
    }

    TSaaSKVKey TSaaSKVKey::From(const TSaaSSubkey& sk) {
        return From(sk.GetType(), sk.GetKey());
    }

    TSaaSKVKey TSaaSKVKey::From(ESaaSSubkeyType sst, TStringBuf sk) {
        auto h = CityHash128(sk);
        return TSaaSKVKey(CombineHashes((ui64)sst, h.first), h.second);
    }

    TSaaSKVKey TSaaSKVKey::From(const TSaaSKey& key) {
        TSaaSKVKey res;
        for (const auto& sk : key) {
            res.Append(TSaaSKVKey::From(sk));
        }
        return res;
    }

    TSaaSKVKey TSaaSKVKey::Parse(TStringBuf subkeyStr) {
        Y_ENSURE(subkeyStr.size() == HASH128_HEX_SIZE, "expected " << HASH128_HEX_SIZE << ", found " << subkeyStr.size());
        THash128 hash128;
        HexDecode(subkeyStr.data(), subkeyStr.size(), &hash128);
        hash128 = FixKeyEndiannes(hash128);
        return TSaaSKVKey(hash128);
    }
}

template <>
NQueryDataSaaS::TSaaSKVKey FromStringImpl<NQueryDataSaaS::TSaaSKVKey, char>(const char* data, size_t len) {
    using namespace NQueryDataSaaS;
    return TSaaSKVKey::Parse({data, len});
}

template <>
void Out<NQueryDataSaaS::TSaaSKVKey>(IOutputStream& o, const NQueryDataSaaS::TSaaSKVKey& k) {
    using namespace NQueryDataSaaS;
    o.Write(k.AsString());
}


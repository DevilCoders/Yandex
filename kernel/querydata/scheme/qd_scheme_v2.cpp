#include "qd_scheme.h"

#include <kernel/querydata/common/querydata_traits.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <util/stream/output.h>

namespace NQueryData {

    const TStringBuf SC_2_TIMESTAMP{TStringBuf("Timestamp")};
    const TStringBuf SC_2_NAMESPACE{TStringBuf("Namespace")};
    const TStringBuf SC_2_TRIE_NAME{TStringBuf("TrieName")};
    const TStringBuf SC_2_HOST_NAME{TStringBuf("HostName")};
    const TStringBuf SC_2_SHARD_NUMBER{TStringBuf("ShardNumber")};
    const TStringBuf SC_2_SHARDS_TOTAL{TStringBuf("ShardsTotal")};
    const TStringBuf SC_2_IS_REALTIME{TStringBuf("IsRealtime")};
    const TStringBuf SC_2_IS_COMMON{TStringBuf("IsCommon")};
    const TStringBuf SC_2_KEY{TStringBuf("Key")};
    const TStringBuf SC_2_KEY_TYPE{TStringBuf("KeyType")};
    const TStringBuf SC_2_VALUE{TStringBuf("Value")};

    const TStringBuf SC_3_META{TStringBuf("Meta")};
    const TStringBuf SC_3_DATA{TStringBuf("Data")};

    void FillKey(NSc::TValue& key, NSc::TValue& keytype, TStringBuf strkey, EKeyType intkeytype) {
        key.Push() = strkey;
        keytype.Push() = KeyTypeDescrById(intkeytype).Name;
    }

    void QueryData2SchemeV2(NSc::TValue& val, const TQueryData& qd) {
        val.SetArray();

        for (ui32 i = 0, sz = qd.SourceFactorsSize(); i < sz; ++i) {
            const TSourceFactors& sf = qd.GetSourceFactors(i);
            NSc::TValue& item = val.Push();

            item[SC_2_TIMESTAMP] = GetTimestampFromVersion(sf.GetVersion());
            item[SC_2_NAMESPACE] = sf.GetSourceName();
            item[SC_2_TRIE_NAME] = sf.GetTrieName();
            item[SC_2_HOST_NAME] = sf.GetHostName();
            item[SC_2_SHARD_NUMBER] = sf.GetShardNumber();
            item[SC_2_SHARDS_TOTAL] = sf.GetShardsTotal();
            item[SC_2_IS_REALTIME] = sf.GetRealTime();

            if (sf.GetCommon()) {
                item[SC_2_IS_COMMON] = 1;
            } else {
                NSc::TValue& key = item[SC_2_KEY].SetArray();
                NSc::TValue& keytype = item[SC_2_KEY_TYPE].SetArray();

                FillKey(key, keytype, sf.GetSourceKey(), sf.GetSourceKeyType());

                for (ui32 k = 0, ksz = sf.SourceSubkeysSize(); k < ksz; ++k) {
                    const TSourceSubkey& sk = sf.GetSourceSubkeys(k);
                    FillKey(key, keytype, sk.GetKey(), sk.GetType());
                }
            }

            DoFillValue(item[SC_2_VALUE].SetDict(), sf, true);
        }
    }

    NSc::TValue QueryData2SchemeV2(const TQueryData& qd) {
        NSc::TValue res;
        QueryData2SchemeV2(res, qd);
        return res;
    }

    void QueryData2SchemeV3(NSc::TValue& val, const TQueryData& qd) {
        val.Add(SC_3_META).SetDict().MergeUpdateJson(qd.GetMeta().GetJson());
        QueryData2SchemeV2(val.Add(SC_3_DATA), qd);
    }

}

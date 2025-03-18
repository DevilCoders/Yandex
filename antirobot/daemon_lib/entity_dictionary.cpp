#include "entity_dictionary.h"

#include <antirobot/idl/factors.pb.h>
#include <antirobot/lib/ip_interval.h>

#include <google/protobuf/messagext.h>

#include <util/network/ip.h>

using namespace NAntiRobot;

template <>
void In<TEntityDictionary>(IInputStream& in, TEntityDictionary& dictionary) {
    NFeaturesProto::TFloatRecord record;
    NFeaturesProto::THeader header;
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TFloatRecord",
            "LoadError TEntityDictionary invalid header");
    size_t num = header.GetNum();

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
        dictionary.Map[record.GetKey()] = record.GetValue();
        --num;
    }

    Y_ENSURE(num == 0, "LoadError TEntityDictionary num mismatch");
}

template <>
void In<THashDictionary>(IInputStream& in, THashDictionary& dictionary) {
    NFeaturesProto::TCityHash64FloatRecord record;
    NFeaturesProto::THeader header;
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TCityHash64FloatRecord",
            "LoadError TCityHash64FloatRecord invalid header");
    size_t num = header.GetNum();

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
        dictionary.Map[record.GetKey()] = record.GetValue();
        --num;
    }
    Y_ENSURE(num == 0, "LoadError TCityHash64FloatRecord num mismatch");
}

template <>
void In<TMarketJwsStatsDictionary>(IInputStream& in, TMarketJwsStatsDictionary& dictionary) {
    NFeaturesProto::TMarketJwsStatesStats record;
    NFeaturesProto::THeader header;
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TMarketJwsStatesStats",
            "LoadError TMarketJwsStatesStats invalid header");
    size_t num = header.GetNum();

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
        dictionary.Map[record.GetKey()] = {
            .DefaultExpiredRatio    = record.GetDefaultExpiredRatio(),
            .DefaultRatio           = record.GetDefaultRatio(),
            .JwsStateIsInvalidRatio = record.GetJwsStateIsInvalidRatio(),
            .SuspExpiredRatio       = record.GetSuspExpiredRatio(),
            .SuspRatio              = record.GetSuspRatio(),
            .ValidExpiredRatio      = record.GetValidExpiredRatio(),
            .ValidRatio             = record.GetValidRatio(),
        };
        --num;
    }
    Y_ENSURE(num == 0, "LoadError TMarketJwsStatesStats num mismatch");
}

template <>
void In<TMarketStatsDictionary>(IInputStream& in, TMarketStatsDictionary& dictionary) {
    NFeaturesProto::TMarketStats record;
    NFeaturesProto::THeader header;
    NProtoBuf::io::TCopyingInputStreamAdaptor adaptor(&in);

    Y_ENSURE(NProtoBuf::io::ParseFromZeroCopyStreamSeq(&header, &adaptor) && header.GetHeader() == "TMarketStats",
            "LoadError TMarketStats invalid header");
    size_t num = header.GetNum();

    while (NProtoBuf::io::ParseFromZeroCopyStreamSeq(&record, &adaptor)) {
        dictionary.Map[record.GetKey()] = {
            .BlockedCntRatio        = static_cast<float>(record.GetBlockedCntRatio()),
            .CatalogReqsCntRatio    = static_cast<float>(record.GetCatalogReqsCntRatio()),
            .EnemyCntRatio          = static_cast<float>(record.GetEnemyCntRatio()),
            .EnemyRedirectsCntRatio  = static_cast<float>(record.GetEnemyRedirectsCntRatio()),
            .FuidCntRatio           = static_cast<float>(record.GetFuidCntRatio()),
            .HostingCntRatio        = static_cast<float>(record.GetHostingCntRatio()),
            .IcookieCntRatio        = static_cast<float>(record.GetIcookieCntRatio()),
            .Ipv4CntRatio           = static_cast<float>(record.GetIpv4CntRatio()),
            .Ipv6CntRatio           = static_cast<float>(record.GetIpv6CntRatio()),
            .LoginCntRatio          = static_cast<float>(record.GetLoginCntRatio()),
            .MobileCntRatio         = static_cast<float>(record.GetMobileCntRatio()),
            .OtherHandlesReqsCntRatio   = static_cast<float>(record.GetOtherHandlesReqsCntRatio()),
            .ProductReqsCntRatio    = static_cast<float>(record.GetProductReqsCntRatio()),
            .ProxyCntRatio          = static_cast<float>(record.GetProxyCntRatio()),
            .RefererIsEmptyCntRatio = static_cast<float>(record.GetRefererIsEmptyCntRatio()),
            .RefererIsNotYandexCntRatio = static_cast<float>(record.GetRefererIsNotYandexCntRatio()),
            .RefererIsYandexCntRatio    = static_cast<float>(record.GetRefererIsYandexCntRatio()),
            .RobotsCntRatio         = static_cast<float>(record.GetRobotsCntRatio()),
            .SearchReqsCntRatio     = static_cast<float>(record.GetSearchReqsCntRatio()),
            .SpravkaCntRatio        = static_cast<float>(record.GetSpravkaCntRatio()),
            .TorCntRatio            = static_cast<float>(record.GetTorCntRatio()),
            .VpnCntRatio            = static_cast<float>(record.GetVpnCntRatio()),
            .YndxIpCntRatio         = static_cast<float>(record.GetYndxIpCntRatio()),
        };
        --num;
    }
    Y_ENSURE(num == 0, "LoadError TMarketStats num mismatch");
}

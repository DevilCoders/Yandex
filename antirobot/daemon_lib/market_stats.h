#pragma once

#include <limits>

namespace NAntiRobot {

enum class EJwsStats {
    DefaultExpiredRatio /* "DefaultExpiredRatio" */,
    DefaultRatio        /* "DefaultRatio" */,
    InvalidRatio        /* "InvalidRatio" */,
    SuspExpiredRatio    /* "SuspExpiredRatio" */,
    SuspRatio           /* "SuspRatio" */,
    ValidExpiredRatio   /* "ValidExpiredRatio" */,
    ValidRatio          /* "ValidRatio" */,
    Count,
};

struct TMarketJwsStatesStats {
    float DefaultExpiredRatio = std::numeric_limits<float>::quiet_NaN();
    float DefaultRatio = std::numeric_limits<float>::quiet_NaN();
    float JwsStateIsInvalidRatio = std::numeric_limits<float>::quiet_NaN();
    float SuspExpiredRatio = std::numeric_limits<float>::quiet_NaN();
    float SuspRatio = std::numeric_limits<float>::quiet_NaN();
    float ValidExpiredRatio = std::numeric_limits<float>::quiet_NaN();
    float ValidRatio = std::numeric_limits<float>::quiet_NaN();
};

enum class EMarketStats {
    BlockedCntRatio             /* "BlockedCntRatio" */,
    CatalogReqsCntRatio         /* "CatalogReqsCntRatio" */,
    EnemyCntRatio               /* "EnemyCntRatio" */,
    EnemyRedirectsCntRatio      /* "EnemyRedirectsCntRatio" */,
    FuidCntRatio                /* "FuidCntRatio" */,
    HostingCntRatio             /* "HostingCntRatio" */,
    IcookieCntRatio             /* "IcookieCntRatio" */,
    Ipv4CntRatio                /* "Ipv4CntRatio" */,
    Ipv6CntRatio                /* "Ipv6CntRatio" */,
    LoginCntRatio               /* "LoginCntRatio" */,
    MobileCntRatio              /* "MobileCntRatio" */,
    OtherHandlesReqsCntRatio    /* "OtherHandlesReqsCntRatio" */,
    ProductReqsCntRatio         /* "ProductReqsCntRatio" */,
    ProxyCntRatio               /* "ProxyCntRatio" */,
    RefererIsEmptyCntRatio      /* "RefererIsEmptyCntRatio" */,
    RefererIsNotYandexCntRatio  /* "RefererIsNotYandexCntRatio" */,
    RefererIsYandexCntRatio     /* "RefererIsYandexCntRatio" */,
    RobotsCntRatio              /* "RobotsCntRatio" */,
    SearchReqsCntRatio          /* "SearchReqsCntRatio" */,
    SpravkaCntRatio             /* "SpravkaCntRatio" */,
    TorCntRatio                 /* "TorCntRatio" */,
    VpnCntRatio                 /* "VpnCntRatio" */,
    YndxIpCntRatio              /* "YndxIpCntRatio" */,
    Count,
};

struct TMarketStats {
    float BlockedCntRatio = std::numeric_limits<float>::quiet_NaN();
    float CatalogReqsCntRatio = std::numeric_limits<float>::quiet_NaN();
    float EnemyCntRatio = std::numeric_limits<float>::quiet_NaN();
    float EnemyRedirectsCntRatio = std::numeric_limits<float>::quiet_NaN();
    float FuidCntRatio = std::numeric_limits<float>::quiet_NaN();
    float HostingCntRatio = std::numeric_limits<float>::quiet_NaN();
    float IcookieCntRatio = std::numeric_limits<float>::quiet_NaN();
    float Ipv4CntRatio = std::numeric_limits<float>::quiet_NaN();
    float Ipv6CntRatio = std::numeric_limits<float>::quiet_NaN();
    float LoginCntRatio = std::numeric_limits<float>::quiet_NaN();
    float MobileCntRatio = std::numeric_limits<float>::quiet_NaN();
    float OtherHandlesReqsCntRatio = std::numeric_limits<float>::quiet_NaN();
    float ProductReqsCntRatio = std::numeric_limits<float>::quiet_NaN();
    float ProxyCntRatio = std::numeric_limits<float>::quiet_NaN();
    float RefererIsEmptyCntRatio = std::numeric_limits<float>::quiet_NaN();
    float RefererIsNotYandexCntRatio = std::numeric_limits<float>::quiet_NaN();
    float RefererIsYandexCntRatio = std::numeric_limits<float>::quiet_NaN();
    float RobotsCntRatio = std::numeric_limits<float>::quiet_NaN();
    float SearchReqsCntRatio = std::numeric_limits<float>::quiet_NaN();
    float SpravkaCntRatio = std::numeric_limits<float>::quiet_NaN();
    float TorCntRatio = std::numeric_limits<float>::quiet_NaN();
    float VpnCntRatio = std::numeric_limits<float>::quiet_NaN();
    float YndxIpCntRatio = std::numeric_limits<float>::quiet_NaN();
};

} // namespace NAntiRobot

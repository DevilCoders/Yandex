#pragma once

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NJson {
    class TJsonValue;
}

namespace NSkyboneD::NApi {

    struct TApiConfig {
        TString Endpoint = "http://skyboned.yandex-team.ru";

        TDuration Timeout = TDuration::Seconds(10);

        TDuration InitFailureDelay = TDuration::MilliSeconds(500);
        double ExponentialBase = 2.0;
        size_t MaxAttempts = 5;
    };

    TString AddResource(const TApiConfig& config,
                        const TString& tvmHeader,
                        const NJson::TJsonValue& request);

    void RemoveResource(const TApiConfig& config,
                        const TString& tvmHeader,
                        const TString& rbtorrent,
                        const TString& sourceId);

}

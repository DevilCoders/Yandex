#pragma once

#include <contrib/libs/tbb/include/tbb/concurrent_hash_map.h>

#include <library/cpp/monlib/service/monservice.h>
#include <library/cpp/monlib/dynamic_counters/counters.h>
#include <util/datetime/base.h>

namespace NTokenAgent {
    class TConfig;
    class TMon {
        using TAgeHashMap = tbb::concurrent_hash_map<std::string, TInstant>;

    public:
        static TMon* Get();
        static void Start(ui16 port);
        static void Stop();

        // Counters
        void ResetAge(const std::string& role, const TInstant& time = TInstant::Now());
        void Denied();
        void Request();
        void TokenServiceError();
        void RevokedKey();
        void TpmAgentError();

    private:
        explicit TMon(ui16 port);
        NMonitoring::TMonService2 MonService;
        NMonitoring::TDynamicCounterPtr Counters;
        TAgeHashMap AgeMap;
    };
}

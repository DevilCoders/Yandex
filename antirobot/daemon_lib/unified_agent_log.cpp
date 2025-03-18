#include "unified_agent_log.h"

#include "config_global.h"
#include "fullreq_info.h"

#include <library/cpp/logger/null.h>
#include <library/cpp/logger/thread.h>

#include <util/string/join.h>


namespace NAntiRobot {
    TUnifiedAgentLogBackend::TUnifiedAgentLogBackend(const TString& unifiedAgentUri, const TString& logType, size_t queueLen)
        : Prefix(logType + "_")
    {
        if (unifiedAgentUri.empty()) {
            return;
        }
        if (ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService 
            && EqualToOneOf(logType, "daemonlog", "processor_daemonlog", "cacher_daemonlog")
        ) {
            return;
        }

        auto clientParameters = NUnifiedAgent::TClientParameters(unifiedAgentUri)
            .SetGrpcMaxMessageSize(20 << 20) // 20 Mb
            .SetMaxInflightBytes(200 << 20); // 200 Mb
        NUnifiedAgent::TSessionParameters sessionParameters;
        const THashMap<TString, TString> metaMap{{"log_type", logType}};
        sessionParameters.SetMeta(metaMap);
        auto queueOverflowCallback = [this]() {
            Counters.Inc(ECounter::RecordsOverflow);
        };
        UnifiedAgentBackend = MakeHolder<TOwningThreadedLogBackend>(NUnifiedAgent::MakeLogBackend(clientParameters, sessionParameters).Release(), queueLen, queueOverflowCallback);
    }

    // TODO: переделать на TUnifiedAgentLogBackend
    TUnifiedAgentDaemonLog::TUnifiedAgentDaemonLog(const TString& unifiedAgentUri) {
        ResetBackend(
            unifiedAgentUri.empty() || ANTIROBOT_DAEMON_CONFIG.AsCaptchaApiService ?
                THolder<TLogBackend>(new TNullLogBackend) :
                THolder<TLogBackend>(new TUnifiedAgentLogBackend(unifiedAgentUri, "daemonlog"))
        );
    }

    NProtoBuf::RepeatedField<ui32> ConvertExpHeaderToProto(const TVector<TRequest::TExpInfo>& src) {
        const auto mappedSrc = MakeMappedRange(src, [] (const TRequest::TExpInfo& x) {
            return x.TestId;
        });

        return {mappedSrc.begin(), mappedSrc.end()};
    }
}

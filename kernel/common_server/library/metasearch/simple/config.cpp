#include "config.h"

namespace NSimpleMeta {

    NSimpleMeta::TConfig TConfig::ForRequester() {
        NSimpleMeta::TConfig result;
        result.SetMaxAttempts(1);
        return result;
    }

    NSimpleMeta::TConfig TConfig::ForMetasearch() {
        NSimpleMeta::TConfig result;
        return result;
    }

    NScatter::TSourceOptions TConfig::BuildSourceOptions() const {
        NScatter::TSourceOptions opts;
        opts.EnableIpV6 = true;
        opts.MaxAttempts = GetMaxAttempts();
        opts.AllowDynamicWeights = GetAllowDynamicWeights();
        opts.ConnectTimeouts = {GetConnectTimeout()};
        opts.SendingTimeouts = {GetSendingTimeout()};
        return opts;
    }

    void TConfig::ToString(IOutputStream& os) const {
        os << "TimeoutConnectms: " << ConnectTimeout.MilliSeconds() << Endl;
        os << "TimeoutSendingms: " << SendingTimeout.MilliSeconds() << Endl;
        os << "GlobalTimeout: " << GlobalTimeout.MilliSeconds() << Endl;
        os << "TasksCheckIntervalms: " << TasksCheckInterval.MilliSeconds() << Endl;

        os << "AllowDynamicWeights: " << AllowDynamicWeights << Endl;
        os << "LoggingEnabled: " << LoggingEnabled << Endl;
        os << "ParallelRequestCount: " << ParallelRequestCount << Endl;
        os << "ThreadsSenders: " << ThreadsSenders << Endl;
        os << "MaxAttempts: " << MaxAttempts << Endl;
        os << "MaxReconnections: " << MaxReconnections << Endl;
        os << "MaxKeysPerRequest: " << MaxKeysPerRequest << Endl;
        os << "MaxQueueSize: " << MaxQueueSize << Endl;
        os << "EventLog: " << EventLogPath << Endl;
        os << "ReaskRate: " << ReaskRate << Endl;
        os << "ReqAnsEnabled: " << ReqAnsEnabled << Endl;
        os << "RouteHashType: " << RouteHashType << Endl;
        os << "SourceTimeoutFactor: " << SourceTimeoutFactor << Endl;
        os << "ThreadsStatusChecker: " << ThreadsStatusChecker << Endl;
    }
}

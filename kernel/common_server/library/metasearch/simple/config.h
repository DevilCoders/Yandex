#pragma once

#include <search/meta/scatter/options/options.h>

#include <kernel/daemon/config/daemon_config.h>

#include <library/cpp/eventlog/eventlog.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/yconf/conf.h>

#include <util/datetime/base.h>
#include <util/system/types.h>

namespace NSimpleMeta {

    class TConfigDefaults {
    public:
        static TDuration GetGlobalTimeout() {
            return TDuration::MilliSeconds(300);
        }

        static ui32 GetMaxAttempts() {
            return 3;
        }
    };

    class TConfig {
    public:
        static TConfig ForRequester();
        static TConfig ForMetasearch();

    private:
        TString RouteHashType;
        ui32 ThreadsSenders = 4;
        ui32 ThreadsStatusChecker = 1;
        ui32 MaxQueueSize = 0;
        ui32 MaxAttempts = 3;
        ui32 MaxReconnections = 3;
        ui32 MaxKeysPerRequest = Max<ui32>();
        TDuration SendingTimeout = TDuration::MilliSeconds(20);
        TDuration ConnectTimeout = TDuration::MilliSeconds(20);
        TDuration GlobalTimeout = TDuration::MilliSeconds(300);
        float ReaskRate = (float)Max<ui32>();
        float SourceTimeoutFactor = 1.0f;
        ui32 ParallelRequestCount = 1;
        TDuration TasksCheckInterval = TDuration::MilliSeconds(20);
        bool AllowDynamicWeights = false;
        bool LoggingEnabled = true;
        bool ReqAnsEnabled = false;
        TString EventLogPath;
        mutable TAtomicSharedPtr<TEventLog> EventLog;

    public:
        NScatter::TSourceOptions BuildSourceOptions() const;

        TInstant GetDeadline() const {
            return Now() + GetGlobalTimeout();
        }

        TConfig& SetConnectTimeout(const TDuration value) {
            ConnectTimeout = value;
            return *this;
        }

        TConfig& SetGlobalTimeout(const TDuration value) {
            GlobalTimeout = value;
            return *this;
        }

        TDuration& MutableGlobalTimeout() {
            return GlobalTimeout;
        }

        ui32& MutableMaxAttempts() {
            return MaxAttempts;
        }

        TConfig& SetTasksCheckInterval(const TDuration value) {
            TasksCheckInterval = value;
            return *this;
        }

        TDuration& MutableTasksCheckInterval() {
            return TasksCheckInterval;
        }

        static TConfig ParseFromString(const TString& configStr) {
            TConfig result;
            TAnyYandexConfig config;
            CHECK_WITH_LOG(config.ParseMemory(configStr.data()));
            result.InitFromSection(config.GetRootSection());
            return result;
        }

    public:
        void ToString(IOutputStream& os) const;

        template <class TDefaults = TConfigDefaults>
        void InitFromSection(const TYandexConfig::Section* section) {
            const TYandexConfig::Directives& directives = section->GetDirectives();

            if (!directives.GetValue<TDuration>("ConnectTimeout", ConnectTimeout)) {
                ConnectTimeout = TDuration::MilliSeconds(directives.Value<ui32>("TimeoutConnectms", ConnectTimeout.MilliSeconds()));
            }
            if (!directives.GetValue<TDuration>("SendingTimeout", SendingTimeout)) {
                SendingTimeout = TDuration::MilliSeconds(directives.Value<ui32>("TimeoutSendingms", SendingTimeout.MilliSeconds()));
            }
            GlobalTimeout = TDuration::MilliSeconds(directives.Value<ui32>("GlobalTimeout", TDefaults::GetGlobalTimeout().MilliSeconds()));
            TasksCheckInterval = TDuration::MilliSeconds(directives.Value<ui32>("TasksCheckIntervalms", TasksCheckInterval.MilliSeconds()));

            directives.GetValue("AllowDynamicWeights", AllowDynamicWeights);
            directives.GetValue("LoggingEnabled", LoggingEnabled);
            directives.GetValue("ParallelRequestCount", ParallelRequestCount);
            directives.GetValue("ThreadsSenders", ThreadsSenders);
            MaxAttempts = directives.Value("MaxAttempts", TDefaults::GetMaxAttempts());
            directives.GetValue("MaxReconnections", MaxReconnections);
            directives.GetValue("MaxKeysPerRequest", MaxKeysPerRequest);
            directives.GetValue("MaxQueueSize", MaxQueueSize);
            directives.GetValue("EventLog", EventLogPath);
            directives.GetValue("ReaskRate", ReaskRate);
            directives.GetValue("ReqAnsEnabled", ReqAnsEnabled);
            directives.GetValue("RouteHashType", RouteHashType);
            directives.GetValue("SourceTimeoutFactor", SourceTimeoutFactor);
            directives.GetValue("ThreadsStatusChecker", ThreadsStatusChecker);
            if (!!EventLogPath) {
                EventLog = MakeAtomicShared<TEventLog>(EventLogPath, NEvClass::Factory()->CurrentFormat());
            }
        }

        TConfig& SetMaxReconnections(const ui32 value) {
            MaxReconnections = value;
            return *this;
        }

        TConfig& SetMaxAttempts(const ui32 value) {
            CHECK_WITH_LOG(value);
            MaxAttempts = value;
            return *this;
        }

        const TString& GetRouteHashType() const {
            return RouteHashType;
        }

        TEventLog* GetEventLog() const {
            return EventLog.Get();
        }

        inline const TDuration& GetTasksCheckInterval() const {
            return TasksCheckInterval;
        }

        inline ui32 GetParallelRequestCount() const {
            return ParallelRequestCount;
        }

        inline TDuration GetSendingTimeout() const {
            return SendingTimeout;
        }

        inline TDuration GetConnectTimeout() const {
            return ConnectTimeout;
        }

        inline TDuration GetGlobalTimeout() const {
            return GlobalTimeout;
        }

        inline float GetReaskRate() const {
            return ReaskRate;
        }

        inline float GetSourceTimeoutFactor() const {
            return SourceTimeoutFactor;
        }

        inline ui32 GetMaxReconnections() const {
            return MaxReconnections;
        }

        inline ui32 GetMaxAttempts() const {
            return MaxAttempts;
        }

        inline ui32 GetMaxKeysPerRequest() const {
            return MaxKeysPerRequest;
        }

        inline ui32 GetThreadsSenders() const {
            return ThreadsSenders;
        }

        inline ui32 GetThreadsStatusChecker() const {
            return ThreadsStatusChecker;
        }

        inline ui32 GetMaxQueueSize() const {
            return MaxQueueSize;
        }

        inline bool GetAllowDynamicWeights() const {
            return AllowDynamicWeights;
        }

        inline bool GetLoggingEnabled() const {
            return LoggingEnabled;
        }

        inline bool GetReqAnsEnabled() const {
            return ReqAnsEnabled;
        }

        TInstant CalcRequestDeadline() const {
            return Now() + GetGlobalTimeout();
        }
    };
}

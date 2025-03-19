#pragma once

#include <kernel/daemon/common/guarded_ptr.h>

#include <library/cpp/yconf/conf.h>
#include <library/cpp/http/server/options.h>
#include <library/cpp/json/writer/json_value.h>
#include <library/cpp/logger/global/common.h>

#include <util/system/fhandle.h>

class TAnyYandexConfig: public TYandexConfig {
public:
    bool OnBeginSection(Section& s) override {
        if (!s.Cookie) {
            s.Cookie = new AnyDirectives;
            s.Owner = true;
        }
        return true;
    }
};

const TYandexConfig::Section& GetSubSection(
    const TYandexConfig::Section& section,
    const TString& name);

template <class TSection>
class TSectionParser {
private:
    TSimpleSharedPtr<TSection> Section;
    struct TParsingValidator {
        TParsingValidator(bool parsed, TSection* section)
        {
            if (!parsed) {
                TString errors;
                section->PrintErrors(errors);
                ythrow yexception() << errors;
            }
        }
    };

    TParsingValidator Validator;

protected:
    typedef TSectionParser<TSection> TParser;
    const TYandexConfig::Section* ServerSection;

public:
    TSectionParser(const char* config, const TString& serverName)
        : Section(new TSection)
        , Validator(Section->ParseMemory(config), Section.Get())
        , ServerSection(&GetSubSection(*Section->GetRootSection(), serverName))
    {
    }
};

class TDaemonConfig {
public:
    class THttpOptions : public THttpServerOptions {
    public:
        THttpOptions();
        THttpOptions(const THttpServerOptions& other);
        virtual ~THttpOptions() {}
        TString ToString(const TString& sectionName) const;
        virtual void Init(const TYandexConfig::Directives& directives, bool verifyThreads = true);
        virtual void ToStringImpl(TStringStream& so) const;

    protected:
        TString BindAddress;
    };

    struct TConnection {
        TString Host;
        ui16 Port = 0;
        TDuration ConnectionTimeout;
        TDuration InteractionTimeout;
        void ToStringFields(TStringStream& ss) const;
    };

    class TControllerConfig : public THttpOptions {
    public:
        struct TDMOptions : public TConnection {
            bool Enabled = false;
            TString UriPrefix;
            TString ServiceType;
            TString Slot;
            TDuration Timeout = TDuration::Seconds(30);

            NJson::TJsonValue Serialize() const;
            void Deserialize(const NJson::TJsonValue& json);
            void Init(const TYandexConfig::Section& section);
            TString ToString() const;
        };
    public:
        ui32 LoadConfigsAttemptionsCount = 3;
        ui32 MaxRestartAttemptions = 3;
        bool StartServer = true;
        bool Enabled = true;
        bool AutoStop = false;
        bool ConfigsControl = true;
        bool ReinitLogsOnRereadConfigs = true;
        TString ConfigsRoot;
        TString StateRoot;
        TString Log;

    public:
        TDMOptions DMOptions;
        TControllerConfig();

        virtual void Init(const TYandexConfig::Directives& directives, bool verifyThreads = true) override;
        void Init(const TYandexConfig::Section& section, bool verifyThreads = true);
        TString GetStatusFileName() const;

        ui32 GetLoadConfigsAttemptionsCount() const {
            return LoadConfigsAttemptionsCount;
        }

        ui32 GetMaxRestartAttemptions() const {
            return MaxRestartAttemptions;
        }

    protected:
        virtual void ToStringImpl(TStringStream& so) const override;
    };
public:
    TDaemonConfig() = default;
    TDaemonConfig(const char* config, bool doInitLogs);

    void InitLogs() const;
    TString ToString(const char* sectionName) const;

    const TMap<TString, TString>& GetSpecialMessageProcessors() const {
        return SpecialProcessorsMap;
    }

    inline bool StartAsDaemon() const {
        return !!PidFile;
    }

    template <class T>
    void StartLogging(const TString& logType) const {
        NLoggingImpl::InitLogImpl<T>(logType, LogLevel, LogRotation, StartAsDaemon());
    }

    template <class T>
    void StartLoggingUnfiltered(const TString& logType) const {
        NLoggingImpl::InitLogImpl<T>(logType, LOG_MAX_PRIORITY, LogRotation, StartAsDaemon());
    }

    const TString& GetCType() const {
        return CType;
    }

    const TString& GetService() const {
        return Service;
    }

    bool RestoreCType(const TString& configPath);

    const TControllerConfig& GetController() const {
        return Controller;
    }

    TControllerConfig& GetController() {
        return Controller;
    }

    inline const TString& GetPidFileName() const {
        return PidFile;
    }

    inline const TString& GetMetricsPrefix() const {
        return MetricsPrefix;
    }

    inline const TString& GetMetricsStorage() const {
        return MetricsStorage;
    }

    inline size_t GetMetricsMaxAge() const {
        return MetricsMaxAge;
    }

    inline const TString& GetLoggerType() const {
        return LoggerType;
    }

    inline TDaemonConfig& SetLoggerType(const TString& value) {
        LoggerType = value;
        return *this;
    }

    inline TDaemonConfig& SetLogLevel(const ui32 value) {
        LogLevel = value;
        return *this;
    }

    inline const TString& GetStdOut() const {
        return StdOut;
    }

    inline void SetStdOut(const TString& log) {
        StdOut = log;
    }

    inline const TString& GetStdErr() const {
        return StdErr;
    }

    inline void SetStdErr(const TString& log) {
        StdErr = log;
    }

    inline int GetLogLevel() const {
        return LogLevel;
    }

    inline bool GetLogRotation() const {
        return LogRotation;
    }

    inline const TDuration& GetLiveTime() const {
        return LiveTime;
    }

    void ReopenLog() const;

public:
    typedef std::pair<TString, ui16> TNetworkAddress;
    static TString DefaultEmptyConfig;

    // All daemons will parse these parameters from config
    static TNetworkAddress ParseAddress(
        const TYandexConfig::Directives& directives,
        const TString& prefix = "", const TString& defaultHost = "");
    static THttpServerOptions ParseHttpServerOptions(
        const TYandexConfig::Directives& directives,
        const TString& prefix = "", const TString& defaultHost = "");
    static TConnection ParseConnection(
        const TYandexConfig::Directives& directives,
        const TString& prefix = "");

    TControllerConfig& MutableController() {
        return Controller;
    }

    const TString& GetDatacenterCode() const {
        return DatacenterCode;
    }

private:
    TString PidFile;
    TString MetricsPrefix;
    size_t MetricsMaxAge = 3;
    TString MetricsStorage;
    TString StdOut;
    TString StdErr;
    TString LoggerType;
    bool JsonGlobalLog = false;
    int LogLevel = 7;
    bool LogRotation = false;
    bool LogThreaded = true;
    TDuration LiveTime = TDuration::Zero();
    TString SpecialProcessors;
    TMap<TString, TString> SpecialProcessorsMap;
    TControllerConfig Controller;
    TString CType;
    TString Service;
    TString DatacenterCode;

private:
    void ReplaceDescriptor(const TString& name, FHANDLE fd) const;
};

using TGuardedDaemonConfig = TGuardedPtr<const TDaemonConfig, TRWMutex, TReadGuard>;


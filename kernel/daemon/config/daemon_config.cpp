#include "daemon_config.h"

#include <library/cpp/json/json_reader.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/mediator/global_notifications/system_status.h>
#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/yconf/patcher/config_patcher.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/file.h>
#include <util/string/escape.h>
#include <util/string/vector.h>
#include <util/system/file.h>
#include <util/system/execpath.h>
#include <util/system/hostname.h>

#if defined(_win_)
#define STDOUT_FILENO INVALID_FHANDLE
#define STDERR_FILENO INVALID_FHANDLE
#else
#include <unistd.h>
#include <fcntl.h>
#endif

const TYandexConfig::Section& GetSubSection(
    const TYandexConfig::Section& section,
    const TString& name)
{
    const TYandexConfig::TSectionsMap& children = section.GetAllChildren();
    const TYandexConfig::TSectionsMap::const_iterator iter =
        children.find(name);
    if (iter == children.end()) {
        ythrow yexception() << "Section " << name.Quote() << " was not found";
    }
    return *iter->second;
}

namespace {
    constexpr TStringBuf FORMATTER_DELIMETR("|FORMATTER_DELIMETR|");
    class TJsonLogBackend final : public TLogBackend {
    public:
        TJsonLogBackend(THolder<TLogBackend>&& slave)
            : Slave(std::move(slave))
        {}

        virtual void WriteData(const TLogRecord & rec) override {
            TBufferOutput buf(rec.Len + 100);
            TStringBuf r(rec.Data, rec.Len);
            TStringBuf format, msg;
            r.Split(FORMATTER_DELIMETR, format, msg);
            msg.ChopSuffix("\n");
            buf << format << EscapeC(msg) << "\"}" << Endl;
            Slave->WriteData(TLogRecord(rec.Priority, buf.Buffer().Data(), buf.Buffer().Size()));
        }
        virtual void ReopenLog() override {
            Slave->ReopenLog();
        }
    private:
        THolder<TLogBackend> Slave;
    };

    TStringBuf StripFileName(TStringBuf string) {
        return string.RNextTok(LOCSLASH_C);
    }

    class TJsonLoggerFormatter final : public ILoggerFormatter {
    public:
        void Format(const TLogRecordContext& context, TLogElement& elem) const override {
            elem << "{\"level\": \"" << context.CustomMessage << "\", \"time\": \"" << NLoggingImpl::GetLocalTimeS() << "\", \"source\": \""
                << StripFileName(context.SourceLocation.File) << ":" << context.SourceLocation.Line << "\", ";
            if (context.Priority > TLOG_RESOURCES && !ExitStarted()) {
                elem << "\"resources\": \"" << NLoggingImpl::GetSystemResources() << "\", ";
            }
            elem << "\"text\": \"" << FORMATTER_DELIMETR;
        }
    };
}
void TDaemonConfig::InitLogs() const {
    auto logBackend = CreateLogBackend(NLoggingImpl::PrepareToOpenLog(LoggerType, LogLevel, LogRotation, StartAsDaemon()), (ELogPriority)LogLevel, LogThreaded);
    if (JsonGlobalLog) {
        DoInitGlobalLog(MakeHolder<TJsonLogBackend>(std::move(logBackend)), MakeHolder<TJsonLoggerFormatter>());
    } else {
        DoInitGlobalLog(std::move(logBackend));
    }
    if (!!StdOut && !TFsPath(StdOut).Parent().Exists())
        ythrow yexception() << "directory with stdout redirect file is incorrect: " << TFsPath(StdOut).Parent().GetPath();

    if (!!StdErr && !TFsPath(StdErr).Parent().Exists())
        ythrow yexception() << "directory with stderr redirect file is incorrect: " << TFsPath(StdErr).Parent().GetPath();

    ReplaceDescriptor(StdOut, STDOUT_FILENO);
    ReplaceDescriptor(StdErr, STDERR_FILENO);
}

TDaemonConfig::TDaemonConfig(const char* config, bool doInitLogs) {
    TAnyYandexConfig parsedConfig;
    CHECK_WITH_LOG(parsedConfig.ParseMemory(config));
    const TYandexConfig::Section* ServerSection = &GetSubSection(*parsedConfig.GetRootSection(), "DaemonConfig");
    AssertCorrectConfig(!!ServerSection, "no section DaemonConfig");

    const TYandexConfig::Directives& directives = ServerSection->GetDirectives();
    CType = directives.Value("CType", CType);
    Service = directives.Value("Service", Service);
    DatacenterCode = directives.Value("DatacenterCode", DatacenterCode);
    PidFile = directives.Value("PidFile", PidFile);
    MetricsPrefix = directives.Value("MetricsPrefix", MetricsPrefix);
    MetricsMaxAge = directives.Value("MetricsMaxAge", MetricsMaxAge);
    MetricsStorage = directives.Value("MetricsStorage", MetricsStorage);

    LiveTime = directives.Value("LiveTime", LiveTime);
    LoggerType = directives.Value("LoggerType", LoggerType);
    LogRotation = directives.Value("LogRotation", false);
    directives.GetValue("LogThreaded", LogThreaded);
    SpecialProcessors = directives.Value("SpecialProcessors", SpecialProcessors);

    TVector<TString> processorsInfo = SplitString(SpecialProcessors, ";");
    for (auto i : processorsInfo) {
        if (i.size()) {
            TVector<TString> info = SplitString(i, ":");
            if (info.size() == 0)
                WARNING_LOG << "Incorrect processor info for " << i << Endl;
            else if (info.size() == 1) {
                SpecialProcessorsMap[info[0]] = "";
            } else if (info.size() == 2) {
                SpecialProcessorsMap[info[0]] = info[1];
            } else {
                SpecialProcessorsMap[info[0]] = JoinStrings(info, 1, info.size() - 1, ":");
            }
        }
    }
    if (!directives.GetValue("LogLevel", LogLevel)) {
        ythrow yexception() << "LogLevel was not set";
    }
    directives.GetValue("StdOut", StdOut);
    directives.GetValue("StdErr", StdErr);
    directives.GetValue("JsonGlobalLog", JsonGlobalLog);
    if (doInitLogs) {
        InitLogs();
    }

    TYandexConfig::TSectionsMap sections = ServerSection->GetAllChildren();
    const TYandexConfig::TSectionsMap::const_iterator iter = sections.find("Controller");
    if (iter != sections.end())
        Controller.Init(*iter->second);
}

TString TDaemonConfig::ToString(const char* sectionName) const {
    TStringStream so;
    so << "<" << sectionName << ">" << Endl;
    so << "PidFile:" << PidFile << Endl;
    so << "MetricsPrefix:" << MetricsPrefix << Endl;
    so << "MetricsMaxAge:" << MetricsMaxAge << Endl;
    so << "LogLevel:" << LogLevel << Endl;
    so << "LogRotation:" << LogRotation << Endl;
    so << "LogThreaded:" << LogThreaded << Endl;
    so << "LiveTime: " << LiveTime.Seconds() << Endl;
    so << "SpecialProcessors: " << SpecialProcessors << Endl;
    so << "LoggerType:" << LoggerType << Endl;
    so << "StdOut:" << StdOut << Endl;
    so << "StdErr:" << StdErr << Endl;
    so << "JsonGlobalLog: " << JsonGlobalLog << Endl;
    so << "CType:" << CType << Endl;
    so << "Service: " << Service << Endl;
    so << "DatacenterCode: " << DatacenterCode << Endl;
    so << Controller.ToString("Controller");
    so << "</" << sectionName << ">" << Endl;
    return so.Str();
}

bool TDaemonConfig::RestoreCType(const TString& configPath) {
    if (!GetCType()) {
        TFsPath pathConfig = TFsPath(configPath);
        TFsPath descirptionPath = pathConfig.Parent() / "description";

        if (descirptionPath.Exists()) {
            TFileInput fi(descirptionPath.GetPath());
            NJson::TJsonValue jsonDescription;
            if (NJson::ReadJsonFastTree(fi.ReadAll(), &jsonDescription)) {
                if (jsonDescription["ctype"].IsString()) {
                    CType = jsonDescription["ctype"].GetString();
                }
                if (jsonDescription["service"].IsString()) {
                    Service = jsonDescription["service"].GetString();
                }
                return true;
            }
        }
        return false;
    }
    return true;
}

void TDaemonConfig::ReplaceDescriptor(const TString& name, FHANDLE fd) const
{
    if (!name || name == TStringBuf("console") || name == TStringBuf("null"))
        return;

    TFileHandle stream(fd);
    CHECK_WITH_LOG(stream.LinkTo(TFileHandle(name, OpenAlways | WrOnly | ForAppend | AWUser | ARUser | ARGroup | AROther)));
    stream.Release();
}


void TDaemonConfig::ReopenLog() const
{
    ReplaceDescriptor(StdOut, STDOUT_FILENO);
    ReplaceDescriptor(StdErr, STDERR_FILENO);
}

TDaemonConfig::TNetworkAddress TDaemonConfig::ParseAddress(
    const TYandexConfig::Directives& directives, const TString& prefix,
    const TString& defaultHost)
{
    TString host;
    if (!directives.GetValue(prefix + "Host", host)) {
        if (!!defaultHost)
            host = defaultHost;
    }

    ui16 port = ui16();
    if (!directives.GetValue(prefix + "Port", port)) {
        ythrow yexception() << "Directive " << (prefix + "Port").Quote()
            << " wasn't found";
    }

    return std::make_pair(host, port);
}

template <class TType>
static inline bool GetValue(const TYandexConfig::Directives& directives,
    const char* key, TType& value)
{
    if (directives.find(key) != directives.end()) {
        return directives.GetValue(key, value);
    } else {
        return false;
    }
}

template <class TType>
static inline TType Value(const TYandexConfig::Directives& directives,
    const char* key, TType def)
{
    if (directives.find(key) != directives.end()) {
        return directives.Value(key, def);
    } else {
        return def;
    }
}

THttpServerOptions TDaemonConfig::ParseHttpServerOptions(
    const TYandexConfig::Directives& directives, const TString& prefix,
    const TString& defaultHost)
{
    TNetworkAddress address = ParseAddress(directives, prefix, defaultHost);
    THttpServerOptions options(address.second);
    options.Host = address.first;
    options.MaxConnections = 0;
    GetValue(directives, (prefix + "Threads").data(), options.nThreads);
    GetValue(directives, (prefix + "MaxQueueSize").data(), options.MaxQueueSize);
    GetValue(directives, (prefix + "MaxFQueueSize").data(), options.MaxFQueueSize);
    GetValue(directives, (prefix + "MaxConnections").data(), options.MaxConnections);
    GetValue(directives, (prefix + "KeepAliveEnabled").data(), options.KeepAliveEnabled);
    options.SetClientTimeout(TDuration::MilliSeconds(
        Value(directives, (prefix + "ClientTimeout").data(), 0)));
    GetValue(directives, (prefix + "CompressionEnabled").data(), options.CompressionEnabled);

    return options;
}

TDaemonConfig::TConnection TDaemonConfig::ParseConnection(
    const TYandexConfig::Directives& directives, const TString& prefix)
{
    TNetworkAddress address = ParseAddress(directives, prefix);
    TConnection result;
    result.Host = address.first;
    result.Port = address.second;
    result.ConnectionTimeout = TDuration::MilliSeconds(
        Value(directives, (prefix + "ConnectionTimeout").data(), 30));
    result.InteractionTimeout = TDuration::MilliSeconds(
        Value(directives, (prefix + "InteractionTimeout").data(), 100));
    return result;
}

TString TDaemonConfig::THttpOptions::ToString(const TString& sectionName) const {
    TStringStream so;
    so << Endl << "<" << sectionName << ">" << Endl;
    ToStringImpl(so);
    so << "</" << sectionName << ">" << Endl;
    return so.Str();
}

void TDaemonConfig::THttpOptions::ToStringImpl(TStringStream& so) const {
    so << "Port : " << Port << Endl
        << "Host : " << Host << Endl
        << "Threads : " << nThreads << Endl
        << "MaxQueueSize : " << MaxQueueSize << Endl
        << "MaxFQueueSize : " << MaxFQueueSize << Endl
        << "MaxConnections : " << MaxConnections << Endl
        << "ClientTimeout : " << ClientTimeout.MilliSeconds() << Endl
        << "KeepAliveEnabled : " << KeepAliveEnabled << Endl
        << "CompressionEnabled : " << CompressionEnabled << Endl
        << "BindAddress : " << BindAddress << Endl;
}

void TDaemonConfig::THttpOptions::Init(const TYandexConfig::Directives& directives, bool verifyThreads) {
    *this = TDaemonConfig::ParseHttpServerOptions(directives);
    if (BindAddress = directives.Value("BindAddress", Default<TString>())) {
        for (auto&& address : SplitString(BindAddress, " ")) {
            auto parts = SplitString(address, "@", 2, KEEP_EMPTY_TOKENS);
            VERIFY_WITH_LOG(parts.size(), "Incorrect ExtraBindAddress");

            const TString& host = parts[0];
            const ui16 port = (parts.size() > 1) ? FromString<ui16>(parts[1]) : 0;
            AddBindAddress(host, port);
        }
    }
    if (verifyThreads)
        VERIFY_WITH_LOG (nThreads != 0, "Set count threads in http server, gannet");
}

TDaemonConfig::THttpOptions::THttpOptions(const THttpServerOptions& other)
    : THttpServerOptions(other)
{}

TDaemonConfig::THttpOptions::THttpOptions()
    : THttpServerOptions()
{}

TDaemonConfig::TControllerConfig::TControllerConfig()
    : THttpOptions()
    , ConfigsRoot(TFsPath(GetExecPath()).Parent().Fix().GetPath() + "/configs")
    , StateRoot(TFsPath(GetExecPath()).Parent().Fix().GetPath() + "/state")
{
    Port = 24242;
}

void TDaemonConfig::TControllerConfig::ToStringImpl(TStringStream& so) const {
    if (Enabled)
        THttpOptions::ToStringImpl(so);
    so << "StartServer : " << StartServer << Endl;
    so << "StateRoot : " << StateRoot << Endl;
    so << "Enabled : " << Enabled << Endl;
    so << "AutoStop : " << AutoStop << Endl;
    so << "ConfigsControl : " << ConfigsControl << Endl;
    so << "LoadConfigsAttemptionsCount : " << LoadConfigsAttemptionsCount << Endl;
    so << "MaxRestartAttemptions : " << MaxRestartAttemptions << Endl;
    so << "ReinitLogsOnRereadConfigs: " << ReinitLogsOnRereadConfigs << Endl;
    so << "ConfigsRoot : " << ConfigsRoot << Endl;
    so << "Log : " << Log << Endl;
    so << DMOptions.ToString() << Endl;
}

TString TDaemonConfig::DefaultEmptyConfig = "<DaemonConfig>\n"
"\tLogLevel: 0\n"
"\tLoggerType: null\n"
"</DaemonConfig>";

TString TDaemonConfig::TControllerConfig::GetStatusFileName() const {
    DEBUG_LOG << "Status root is " << StateRoot << Endl;
    return StateRoot + "/controller-state-" + ::ToString(Port);
}

void TDaemonConfig::TControllerConfig::Init(const TYandexConfig::Directives& directives, bool /*verifyThreads*/) {
    GetValue(directives, "Enabled", Enabled);
    if (Enabled) {
        THttpOptions::Init(directives);
    }
    GetValue(directives, "StateRoot", StateRoot);
    GetValue(directives, "StartServer", StartServer);
    GetValue(directives, "AutoStop", AutoStop);
    GetValue(directives, "ConfigsControl", ConfigsControl);
    GetValue(directives, "LoadConfigsAttemptionsCount", LoadConfigsAttemptionsCount);
    GetValue(directives, "MaxRestartAttemptions", MaxRestartAttemptions);
    GetValue(directives, "ReinitLogsOnRereadConfigs", ReinitLogsOnRereadConfigs);
    GetValue(directives, "ConfigsRoot", ConfigsRoot);
    GetValue(directives, "Log", Log);
}

void TDaemonConfig::TControllerConfig::Init(const TYandexConfig::Section& section, bool verifyThreads) {
    Init(section.GetDirectives(), verifyThreads);
    TYandexConfig::TSectionsMap sections = section.GetAllChildren();
    const TYandexConfig::TSectionsMap::const_iterator iter = sections.find("DMOptions");
    if (iter != sections.end())
        DMOptions.Init(*iter->second);
}

void TDaemonConfig::TControllerConfig::TDMOptions::Init(const TYandexConfig::Section& section) {
    const TYandexConfig::Directives& dir = section.GetDirectives();
    GetValue(dir, "Enabled", Enabled);
    if (Enabled)
        *(TConnection*)this = ParseConnection(dir, "");
    GetValue(dir, "UriPrefix", UriPrefix);
    GetValue(dir, "ServiceType", ServiceType);
    GetValue(dir, "Slot", Slot);
    ui32 timeoutS = Timeout.Seconds();
    GetValue(dir, "TimeoutS", timeoutS);
    Timeout = TDuration::Seconds(timeoutS);
}

TString TDaemonConfig::TControllerConfig::TDMOptions::ToString() const {
    TStringStream ss;
    ss << "<DMOptions>" << Endl;
    ss << "Enabled: " << Enabled << Endl;
    ToStringFields(ss);
    ss << "UriPrefix: " << UriPrefix << Endl;
    ss << "ServiceType: " << ServiceType << Endl;
    ss << "Slot: " << Slot << Endl;
    ss << "TimeoutS: " << Timeout.Seconds() << Endl;
    ss << "</DMOptions>" << Endl;
    return ss.Str();
}

NJson::TJsonValue TDaemonConfig::TControllerConfig::TDMOptions::Serialize() const {
    TUnstrictConfig cfg;
    TString str = ToString();
    VERIFY_WITH_LOG(cfg.ParseMemory(str.data()), "invalid  TDaemonConfig::TControllerConfig::TDMOptions::ToString")
    NJson::TJsonValue result;
    cfg.ToJsonPatch(*cfg.GetRootSection()->GetAllChildren().find("DMOptions")->second, result, "");
    return result;
}

void TDaemonConfig::TControllerConfig::TDMOptions::Deserialize(const NJson::TJsonValue& json) {
    NConfigPatcher::TOptions ops;
    ops.Prefix = "DMOptions.";
    TUnstrictConfig cfg;
    TString str = NConfigPatcher::Patch("", json.GetStringRobust(), ops);
    VERIFY_WITH_LOG(cfg.ParseMemory(str.data()), "invalid NConfigPatcher::Patch");
    Init(*cfg.GetRootSection()->GetAllChildren().find("DMOptions")->second);
}

void TDaemonConfig::TConnection::ToStringFields(TStringStream& ss) const {
    ss << "Host: " << Host << Endl;
    ss << "Port: " << Port << Endl;
    ss << "ConnectionTimeout: " << ConnectionTimeout.MilliSeconds() << Endl;
    ss << "InteractionTimeout: " << InteractionTimeout.MilliSeconds() << Endl;
}

#include "config.h"

#include <library/cpp/monlib/service/pages/templates.h>

namespace NCloud::NBlockStore::NClient {

namespace {

////////////////////////////////////////////////////////////////////////////////

TDuration Minutes(ui64 x)
{
    return TDuration::Minutes(x);
}

TDuration Seconds(ui64 x)
{
    return TDuration::Seconds(x);
}

TDuration MSeconds(ui64 x)
{
    return TDuration::MilliSeconds(x);
}

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_CLIENT_CONFIG(xxx)                                          \
    xxx(Host,                   TString,          "localhost"                 )\
    xxx(Port,                   ui32,             0                           )\
    xxx(InsecurePort,           ui32,             9766                        )\
    xxx(MaxMessageSize,         ui32,             64*1024*1024                )\
    xxx(ThreadsCount,           ui32,             1                           )\
                                                                               \
    xxx(RequestTimeout,                 TDuration,      Seconds(30)           )\
    xxx(RetryTimeout,                   TDuration,      Minutes(5)            )\
    xxx(RetryTimeoutIncrement,          TDuration,      MSeconds(500)         )\
    xxx(ConnectionErrorMaxRetryTimeout, TDuration,      MSeconds(100)         )\
    xxx(GrpcReconnectBackoff,           TDuration,      MSeconds(100)         )\
                                                                               \
    xxx(MemoryQuotaBytes,       ui32,             0                           )\
    xxx(SecurePort,             ui32,             0                           )\
    xxx(RootCertsFile,          TString,          {}                          )\
    xxx(CertFile,               TString,          {}                          )\
    xxx(CertPrivateKeyFile,     TString,          {}                          )\
    xxx(AuthToken,              TString,          {}                          )\
    xxx(UnixSocketPath,         TString,          {}                          )\
    xxx(GrpcThreadsLimit,       ui32,             4                           )\
    xxx(InstanceId,             TString,          {}                          )\
    xxx(MaxRequestSize,         ui32,             4*1024*1024                 )\
    xxx(ClientId,               TString,          {}                          )\
    xxx(IpcType,                NProto::EClientIpcType, NProto::IPC_GRPC      )\
    xxx(NbdThreadsCount,        ui32,             1                           )\
    xxx(NbdSocketSuffix,        TString,          {}                          )\
    xxx(NbdStructuredReply,     bool,             false                       )\
    xxx(NbdUseNbsErrors,        bool,             false                       )\
    xxx(RemountDeadline,        TDuration,        MSeconds(500)               )\
    xxx(NvmeDeviceTransportId,  TString,          {}                          )\
    xxx(NvmeDeviceNqn,          TString,          {}                          )\
    xxx(ScsiDeviceUrl,          TString,          {}                          )\
    xxx(ScsiInitiatorIqn,       TString,          {}                          )\
    xxx(RdmaDeviceAddress,      TString,          {}                          )\
    xxx(RdmaDevicePort,         ui32,             0                           )\
// BLOCKSTORE_CLIENT_CONFIG

#define BLOCKSTORE_CLIENT_DECLARE_CONFIG(name, type, value)                    \
    Y_DECLARE_UNUSED static const type Default##name = value;                  \
// BLOCKSTORE_CLIENT_DECLARE_CONFIG

BLOCKSTORE_CLIENT_CONFIG(BLOCKSTORE_CLIENT_DECLARE_CONFIG)

#undef BLOCKSTORE_CLIENT_DECLARE_CONFIG

////////////////////////////////////////////////////////////////////////////////

template <typename TTarget, typename TSource>
TTarget ConvertValue(TSource value)
{
    return static_cast<TTarget>(value);
}

template <>
TDuration ConvertValue<TDuration, ui32>(ui32 value)
{
    return TDuration::MilliSeconds(value);
}

////////////////////////////////////////////////////////////////////////////////

IOutputStream& operator <<(IOutputStream& out, NProto::EClientIpcType ipcType)
{
    switch (ipcType) {
        case NProto::IPC_GRPC:
            return out << "IPC_GRPC";
        case NProto::IPC_NBD:
            return out << "IPC_NBD";
        case NProto::IPC_VHOST:
            return out << "IPC_VHOST";
        case NProto::IPC_NVME:
            return out << "IPC_NVME";
        case NProto::IPC_SCSI:
            return out << "IPC_SCSI";
        case NProto::IPC_RDMA:
            return out << "IPC_RDMA";
        default:
            return out << "(Unknown value " << static_cast<int>(ipcType) << ")";
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TClientAppConfig::TClientAppConfig(NProto::TClientAppConfig appConfig)
    : AppConfig(std::move(appConfig))
    , ClientConfig(AppConfig.GetClientConfig())
    , LogConfig(AppConfig.GetLogConfig())
    , MonitoringConfig(AppConfig.GetMonitoringConfig())
{}

#define BLOCKSTORE_CONFIG_GETTER(name, type, ...)                              \
type TClientAppConfig::Get##name() const                                       \
{                                                                              \
    const auto value = ClientConfig.Get##name();                               \
    return value ? ConvertValue<type>(value) : Default##name;                  \
}                                                                              \
// BLOCKSTORE_CONFIG_GETTER

BLOCKSTORE_CLIENT_CONFIG(BLOCKSTORE_CONFIG_GETTER)

#undef BLOCKSTORE_CONFIG_GETTER

void TClientAppConfig::Dump(IOutputStream& out) const
{
#define BLOCKSTORE_CONFIG_DUMP(name, ...)                                      \
    out << #name << ": " << Get##name() << Endl;                               \
// BLOCKSTORE_CONFIG_DUMP

    BLOCKSTORE_CLIENT_CONFIG(BLOCKSTORE_CONFIG_DUMP);

#undef BLOCKSTORE_CONFIG_DUMP
}

void TClientAppConfig::DumpHtml(IOutputStream& out) const
{
#define BLOCKSTORE_CONFIG_DUMP(name, ...)                                      \
    TABLER() {                                                                 \
        TABLED() { out << #name; }                                             \
        TABLED() { out << Get##name(); }                                       \
    }                                                                          \
// BLOCKSTORE_CONFIG_DUMP

    HTML(out) {
        TABLE_CLASS("table table-condensed") {
            TABLEBODY() {
                BLOCKSTORE_CLIENT_CONFIG(BLOCKSTORE_CONFIG_DUMP);
            }
        }
    }

#undef BLOCKSTORE_CONFIG_DUMP
}

}   // namespace NCloud::NBlockStore::NClient

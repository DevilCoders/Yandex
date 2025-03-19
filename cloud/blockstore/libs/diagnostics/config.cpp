#include "config.h"

#include <library/cpp/monlib/service/pages/templates.h>
#include <library/cpp/protobuf/util/pb_io.h>

namespace NCloud::NBlockStore {

namespace {

////////////////////////////////////////////////////////////////////////////////

#define BLOCKSTORE_DIAGNOSTICS_CONFIG(xxx)                                                               \
    xxx(HostNameScheme,                  NProto::EHostNameScheme, NProto::EHostNameScheme::HOSTNAME_RAW )\
    xxx(BastionNameSuffix,               TString,         "ydb.bastion.cloud.yandex-team.ru"            )\
    xxx(ViewerHostName,                  TString,         "viewer.ydb.yandex-team.ru"                   )\
    xxx(SolomonClusterName,              TString,         ""                                            )\
    xxx(KikimrMonPort,                   ui32,            8765                                          )\
    xxx(NbsMonPort,                      ui32,            8766                                          )\
                                                                                                         \
    xxx(HDDSlowRequestThreshold,                TDuration,       TDuration::Seconds(0)                  )\
    xxx(SSDSlowRequestThreshold,                TDuration,       TDuration::Seconds(0)                  )\
    xxx(NonReplicatedSSDSlowRequestThreshold,   TDuration,       TDuration::MilliSeconds(10)            )\
    xxx(SamplingRate,                    ui32,            0                                             )\
    xxx(SlowRequestSamplingRate,         ui32,            0                                             )\
    xxx(TracesUnifiedAgentEndpoint,      TString,         ""                                            )\
    xxx(TracesSyslogIdentifier,          TString,         ""                                            )\
\
    xxx(ProfileLogTimeThreshold,         TDuration,       TDuration::Seconds(15)                        )\
    xxx(UseAsyncLogger,                  bool,            false                                         )\
    xxx(SsdPerfThreshold,                TPerfThreshold,  {}                                            )\
    xxx(HddPerfThreshold,                TPerfThreshold,  {}                                            )\
    xxx(SolomonUrl,                      TString,         "https://solomon.yandex-team.ru"              )\
    xxx(SolomonProject,                  TString,         "nbs"                                         )\
    xxx(UnsafeLWTrace,                   bool,            false                                         )\
    xxx(LWTraceDebugInitializationQuery, TString,         ""                                            )\
    xxx(SsdPerfSettings,                TVolumePerfSettings,  {}                                        )\
    xxx(HddPerfSettings,                TVolumePerfSettings,  {}                                        )\
    xxx(NonreplPerfSettings,            TVolumePerfSettings,  {}                                        )\
    xxx(Mirror2PerfSettings,            TVolumePerfSettings,  {}                                        )\
    xxx(Mirror3PerfSettings,            TVolumePerfSettings,  {}                                        )\
    xxx(LocalSSDPerfSettings,           TVolumePerfSettings,  {}                                        )\
    xxx(ExpectedIoParallelism,          ui32,                 32                                        )\
    xxx(LWTraceShuttleCount,            ui32,                 2000                                      )\
                                                                                                         \
    xxx(CpuWaitFilename,            TString, "/sys/fs/cgroup/cpu/system.slice/nbs.service/cpuacct.wait" )\
// BLOCKSTORE_DIAGNOSTICS_CONFIG

#define BLOCKSTORE_DIAGNOSTICS_DECLARE_CONFIG(name, type, value)               \
    Y_DECLARE_UNUSED static const type Default##name = value;                  \
// BLOCKSTORE_DIAGOSTICS_DECLARE_CONFIG

BLOCKSTORE_DIAGNOSTICS_CONFIG(BLOCKSTORE_DIAGNOSTICS_DECLARE_CONFIG)

#undef BLOCKSTORE_DIAGNOSTICS_DECLARE_CONFIG

////////////////////////////////////////////////////////////////////////////////

template <typename TTarget, typename TSource>
TTarget ConvertValue(const TSource& value)
{
    return static_cast<TTarget>(value);
}

template <>
TDuration ConvertValue<TDuration, ui32>(const ui32& value)
{
    return TDuration::MilliSeconds(value);
}

template <>
TPerfThreshold ConvertValue<TPerfThreshold, NProto::TVolumePerfThreshold>(
    const NProto::TVolumePerfThreshold& value)
{
    return {
        value.GetReadThreshold().GetThreshold(),
        value.GetReadThreshold().GetPercentile(),
        value.GetWriteThreshold().GetThreshold(),
        value.GetWriteThreshold().GetPercentile()
    };
}

template <>
TVolumePerfSettings ConvertValue<TVolumePerfSettings, NProto::TVolumePerfSettings>(
    const NProto::TVolumePerfSettings& value)
{
    return TVolumePerfSettings(value);
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

TDiagnosticsConfig::TDiagnosticsConfig(NProto::TDiagnosticsConfig diagnosticsConfig)
    : DiagnosticsConfig(std::move(diagnosticsConfig))
{}

#define BLOCKSTORE_CONFIG_GETTER(name, type, ...)                                    \
type TDiagnosticsConfig::Get##name() const                                           \
{                                                                                    \
    auto has = DiagnosticsConfig.Has##name();                                        \
    return has ? ConvertValue<type>(DiagnosticsConfig.Get##name()) : Default##name;  \
}                                                                                    \
// BLOCKSTORE_CONFIG_GETTER

BLOCKSTORE_DIAGNOSTICS_CONFIG(BLOCKSTORE_CONFIG_GETTER);

#undef BLOCKSTORE_CONFIG_GETTER

void TDiagnosticsConfig::Dump(IOutputStream& out) const
{
#define BLOCKSTORE_CONFIG_DUMP(name, ...)                                      \
    out << #name << ": " << Get##name() << Endl;                               \
// BLOCKSTORE_CONFIG_DUMP

    BLOCKSTORE_DIAGNOSTICS_CONFIG(BLOCKSTORE_CONFIG_DUMP);

#undef BLOCKSTORE_CONFIG_DUMP
}

void TDiagnosticsConfig::DumpHtml(IOutputStream& out) const
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
                BLOCKSTORE_DIAGNOSTICS_CONFIG(BLOCKSTORE_CONFIG_DUMP);
            }
        }
    }

#undef BLOCKSTORE_CONFIG_DUMP
}

}   // namespace NCloud::NBlockStore

////////////////////////////////////////////////////////////////////////////////

template <>
void Out<NCloud::NBlockStore::NProto::EHostNameScheme>(
    IOutputStream& out,
    NCloud::NBlockStore::NProto::EHostNameScheme scheme)
{
    out << NCloud::NBlockStore::NProto::EHostNameScheme_Name(scheme);
}

template <>
void Out<NCloud::NBlockStore::TPerfThreshold>(
    IOutputStream& out,
    const NCloud::NBlockStore::TPerfThreshold& value)
{
    NCloud::NBlockStore::NProto::TVolumePerfThreshold v;
    v.MutableReadThreshold()->SetThreshold(value.ReadThreshold);
    v.MutableReadThreshold()->SetPercentile(value.ReadPercentile);
    v.MutableWriteThreshold()->SetThreshold(value.WriteThreshold);
    v.MutableWriteThreshold()->SetPercentile(value.WritePercentile);

    SerializeToTextFormat(v, out);
}

template <>
void Out<NCloud::NBlockStore::TVolumePerfSettings>(
    IOutputStream& out,
    const NCloud::NBlockStore::TVolumePerfSettings& value)
{
    NCloud::NBlockStore::NProto::TVolumePerfSettings v;
    v.MutableRead()->SetIops(value.ReadIops);
    v.MutableRead()->SetBandwidth(value.ReadBandwidth);
    v.MutableWrite()->SetIops(value.WriteIops);
    v.MutableWrite()->SetBandwidth(value.WriteBandwidth);

    SerializeToTextFormat(v, out);
}

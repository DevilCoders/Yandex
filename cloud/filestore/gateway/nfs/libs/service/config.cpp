#include "config.h"

#include <library/cpp/monlib/service/pages/templates.h>

#include <util/system/compiler.h>

namespace NCloud::NFileStore::NGateway {

namespace {

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_GATEWAY_CONFIG(xxx)                                          \
    xxx(SessionRetryTimeout,    TDuration,      TDuration::Seconds(1)         )\
    xxx(SessionPingTimeout,     TDuration,      TDuration::Seconds(1)         )\
// FILESTORE_GATEWAY_CONFIG

#define FILESTORE_GATEWAY_DECLARE_CONFIG(name, type, value)                    \
    Y_DECLARE_UNUSED static const type Default##name = value;                  \
// FILESTORE_GATEWAY_DECLARE_CONFIG

FILESTORE_GATEWAY_CONFIG(FILESTORE_GATEWAY_DECLARE_CONFIG)

#undef FILESTORE_GATEWAY_DECLARE_CONFIG

static const TDuration DefaultRequestTimeout = TDuration::Seconds(30);

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

template <typename T>
bool IsEmpty(const T& t)
{
    return !t;
}

template <typename T>
void DumpImpl(const T& t, IOutputStream& os)
{
    os << t;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_CONFIG_GETTER(name, type, ...)                               \
type TFileStoreServiceConfig::Get##name() const                                \
{                                                                              \
    const auto value = ProtoConfig.Get##name();                                \
    return !IsEmpty(value) ? ConvertValue<type>(value) : Default##name;        \
}                                                                              \
// FILESTORE_CONFIG_GETTER

FILESTORE_GATEWAY_CONFIG(FILESTORE_CONFIG_GETTER)

#undef FILESTORE_CONFIG_GETTER

void TFileStoreServiceConfig::Dump(IOutputStream& out) const
{
#define FILESTORE_CONFIG_DUMP(name, ...)                                       \
    out << #name << ": ";                                                      \
    DumpImpl(Get##name(), out);                                                \
    out << Endl;                                                               \
// FILESTORE_CONFIG_DUMP

    FILESTORE_GATEWAY_CONFIG(FILESTORE_CONFIG_DUMP);

#undef FILESTORE_CONFIG_DUMP
}

void TFileStoreServiceConfig::DumpHtml(IOutputStream& out) const
{
#define FILESTORE_CONFIG_DUMP(name, ...)                                       \
    TABLER() {                                                                 \
        TABLED() { out << #name; }                                             \
        TABLED() { DumpImpl(Get##name(), out); }                               \
    }                                                                          \
// FILESTORE_CONFIG_DUMP

    HTML(out) {
        TABLE_CLASS("table table-condensed") {
            TABLEBODY() {
                FILESTORE_GATEWAY_CONFIG(FILESTORE_CONFIG_DUMP);
            }
        }
    }

#undef FILESTORE_CONFIG_DUMP
}

TDuration TFileStoreServiceConfig::GetRequestTimeout() const
{
    const auto& clientConfig = ProtoConfig.GetClientConfig();
    const auto value = clientConfig.GetRequestTimeout();
    return !IsEmpty(value) ? ConvertValue<TDuration>(value) : DefaultRequestTimeout;
}

}   // namespace NCloud::NFileStore::NGateway

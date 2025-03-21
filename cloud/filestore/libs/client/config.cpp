#include "config.h"

#include <library/cpp/monlib/service/pages/templates.h>

namespace NCloud::NFileStore::NClient {

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

#define FILESTORE_CLIENT_CONFIG(xxx)                                           \
    xxx(Host,                   TString,        "localhost"                   )\
    xxx(Port,                   ui32,           9021                          )\
                                                                               \
    xxx(ThreadsCount,           ui32,           1                             )\
    xxx(RequestTimeout,         TDuration,      Seconds(30)                   )\
                                                                               \
    xxx(RetryTimeout,                   TDuration,      Minutes(10)           )\
    xxx(RetryTimeoutIncrement,          TDuration,      MSeconds(500)         )\
    xxx(ConnectionErrorMaxRetryTimeout, TDuration,      MSeconds(100)         )\
// FILESTORE_CLIENT_CONFIG

#define FILESTORE_CLIENT_DECLARE_CONFIG(name, type, value)                     \
    Y_DECLARE_UNUSED static const type Default##name = value;                  \
// FILESTORE_CLIENT_DECLARE_CONFIG

FILESTORE_CLIENT_CONFIG(FILESTORE_CLIENT_DECLARE_CONFIG)

#undef FILESTORE_CLIENT_DECLARE_CONFIG

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_SESSION_CONFIG(xxx)                                          \
    xxx(FileSystemId,           TString,        {}                            )\
    xxx(ClientId,               TString,        {}                            )\
    xxx(SessionPingTimeout,     TDuration,      Seconds(1)                    )\
    xxx(SessionRetryTimeout,    TDuration,      Seconds(1)                    )\
// FILESTORE_SESSION_CONFIG

#define FILESTORE_SESSION_DECLARE_CONFIG(name, type, value)                    \
    Y_DECLARE_UNUSED static const type Default##name = value;                  \
// FILESTORE_SESSION_DECLARE_CONFIG

FILESTORE_SESSION_CONFIG(FILESTORE_SESSION_DECLARE_CONFIG)

#undef FILESTORE_SESSION_DECLARE_CONFIG

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
type TClientConfig::Get##name() const                                          \
{                                                                              \
    const auto value = ProtoConfig.Get##name();                                \
    return !IsEmpty(value) ? ConvertValue<type>(value) : Default##name;        \
}                                                                              \
// FILESTORE_CONFIG_GETTER

FILESTORE_CLIENT_CONFIG(FILESTORE_CONFIG_GETTER)

#undef FILESTORE_CONFIG_GETTER

void TClientConfig::Dump(IOutputStream& out) const
{
#define FILESTORE_CONFIG_DUMP(name, ...)                                       \
    out << #name << ": ";                                                      \
    DumpImpl(Get##name(), out);                                                \
    out << Endl;                                                               \
// FILESTORE_CONFIG_DUMP

    FILESTORE_CLIENT_CONFIG(FILESTORE_CONFIG_DUMP);

#undef FILESTORE_CONFIG_DUMP
}

void TClientConfig::DumpHtml(IOutputStream& out) const
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
                FILESTORE_CLIENT_CONFIG(FILESTORE_CONFIG_DUMP);
            }
        }
    }

#undef FILESTORE_CONFIG_DUMP
}

////////////////////////////////////////////////////////////////////////////////

#define FILESTORE_CONFIG_GETTER(name, type, ...)                               \
type TSessionConfig::Get##name() const                                         \
{                                                                              \
    const auto value = ProtoConfig.Get##name();                                \
    return !IsEmpty(value) ? ConvertValue<type>(value) : Default##name;        \
}                                                                              \
// FILESTORE_CONFIG_GETTER

FILESTORE_SESSION_CONFIG(FILESTORE_CONFIG_GETTER)

#undef FILESTORE_CONFIG_GETTER

void TSessionConfig::Dump(IOutputStream& out) const
{
#define FILESTORE_CONFIG_DUMP(name, ...)                                       \
    out << #name << ": ";                                                      \
    DumpImpl(Get##name(), out);                                                \
    out << Endl;                                                               \
// FILESTORE_CONFIG_DUMP

    FILESTORE_SESSION_CONFIG(FILESTORE_CONFIG_DUMP);

#undef FILESTORE_CONFIG_DUMP
}

void TSessionConfig::DumpHtml(IOutputStream& out) const
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
                FILESTORE_SESSION_CONFIG(FILESTORE_CONFIG_DUMP);
            }
        }
    }

#undef FILESTORE_CONFIG_DUMP
}
}   // namespace NCloud::NFileStore::NClient

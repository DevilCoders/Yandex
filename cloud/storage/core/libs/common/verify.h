#pragma once

#include <library/cpp/logger/log.h>

#include <util/generic/singleton.h>
#include <util/string/builder.h>
#include <util/system/spinlock.h>
#include <util/system/yassert.h>

namespace NCloud {

////////////////////////////////////////////////////////////////////////////////

struct TVerifyLog
{
    TLog Log;
    TAdaptiveLock Lock;
};

inline void SetVerifyLogBackend(std::unique_ptr<TLogBackend> backend)
{
    auto* vl = Singleton<TVerifyLog>();
    with_lock (vl->Lock) {
        vl->Log.ResetBackend(THolder<TLogBackend>(backend.release()));
    }
}

////////////////////////////////////////////////////////////////////////////////

struct TWellKnownEntityTypes
{
    static constexpr TStringBuf DISK = "Disk";
    static constexpr TStringBuf TABLET = "Tablet";
    static constexpr TStringBuf CLIENT = "Client";
};

}   // namespace NCloud

#define STORAGE_VERIFY_C(expr, entityType, entityId, message)                  \
    do {                                                                       \
        if (Y_UNLIKELY(!(expr))) {                                             \
            TStringBuilder sb;                                                 \
            sb << "Problem with " << entityType << " " << entityId             \
                << ": " << #expr;                                              \
            auto m = (message);                                                \
            if (m) {                                                           \
                sb << "\nMessage: " << m;                                      \
            }                                                                  \
            auto* vl = Singleton<TVerifyLog>();                                \
            with_lock (vl->Lock) {                                             \
                vl->Log.Write(                                                 \
                    ELogPriority::TLOG_EMERG,                                  \
                    TStringBuilder() << entityType                             \
                        << "\t" << entityId << "\n");                          \
            }                                                                  \
            Y_FAIL("%s", sb.c_str());                                          \
        }                                                                      \
    } while (false)                                                            \
// STORAGE_VERIFY_C

#define STORAGE_VERIFY(expr, entityType, entityId)                             \
    STORAGE_VERIFY_C(expr, entityType, entityId, "");                          \
// STORAGE_VERIFY

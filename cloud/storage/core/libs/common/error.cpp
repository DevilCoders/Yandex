#include "error.h"

#include <cloud/storage/core/libs/common/helpers.h>

#include <util/stream/format.h>
#include <util/stream/output.h>
#include <util/stream/str.h>

namespace NCloud {

namespace {

////////////////////////////////////////////////////////////////////////////////

void FormatResultCodeRaw(IOutputStream& out, ui32 code)
{
    out << static_cast<ESeverityCode>(SUCCEEDED(code) ? 0 : 1) << " | ";

    ui32 facility = FACILITY_FROM_CODE(code);
    if (facility < FACILITY_MAX) {
        out << static_cast<EFacilityCode>(facility);
    } else {
        out << "FACILITY_UNKNOWN";
    }

    out << " | " << STATUS_FROM_CODE(code);
}

void FormatResultCodePretty(IOutputStream& out, ui32 code)
{
    // TODO
    try {
        out << static_cast<EWellKnownResultCodes>(code);
    } catch (...) {
        FormatResultCodeRaw(out, code);
    }
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

EErrorKind GetErrorKind(const NProto::TError& e)
{
    ui32 code = e.GetCode();

    if (SUCCEEDED(code)) {
        return EErrorKind::Success;
    }

    // TODO: do not retrieve E_TIMEOUT, E_THROTTLED from error message
    // on client side: https://st.yandex-team.ru/NBS-568
    if (code == E_REJECTED) {
        if (e.GetMessage() == "Throttled") {
            code = E_BS_THROTTLED;
        } else if (e.GetMessage() == "Timeout") {
            code = E_TIMEOUT;
        }
    }

    if (HasProtoFlag(e.GetFlags(), NProto::EF_SILENT)
        || code == E_IO_SILENT) // TODO: https://st.yandex-team.ru/NBS-3124#622886b937bf95501db66aad
    {
        return EErrorKind::ErrorSilent;
    }

    switch (code) {
        case E_REJECTED:
        case E_TIMEOUT:
        case E_BS_OUT_OF_SPACE:
        case E_FS_OUT_OF_SPACE:
            return EErrorKind::ErrorRetriable;

        case E_BS_THROTTLED:
        case E_FS_THROTTLED:
            return EErrorKind::ErrorThrottling;

        case E_FS_INVALID_SESSION:
        case E_BS_INVALID_SESSION:
            return EErrorKind::ErrorSession;
    }

    if (FACILITY_FROM_CODE(code) == FACILITY_GRPC ||
        FACILITY_FROM_CODE(code) == FACILITY_SYSTEM)
    {
        // system/network errors should be retriable
        return EErrorKind::ErrorRetriable;
    }

    if (FACILITY_FROM_CODE(code) == FACILITY_KIKIMR) {
        switch (STATUS_FROM_CODE(code)) {
            case 1:  // NKikimrProto::ERROR
            case 3:  // NKikimrProto::TIMEOUT
            case 4:  // NKikimrProto::RACE
            case 6:  // NKikimrProto::BLOCKED
            case 7:  // NKikimrProto::NOTREADY
            case 12: // NKikimrProto::DEADLINE
            case 20: // NKikimrProto::NOT_YET
                return EErrorKind::ErrorRetriable;
        }
    }

    if (FACILITY_FROM_CODE(code) == FACILITY_SCHEMESHARD) {
        switch (STATUS_FROM_CODE(code)) {
            case 13: // NKikimrScheme::StatusNotAvailable
            case 8:  // NKikimrScheme::StatusMultipleModifications
                return EErrorKind::ErrorRetriable;
        }
    }

    if (FACILITY_FROM_CODE(code) == FACILITY_TXPROXY) {
        switch (STATUS_FROM_CODE(code)) {
            case 16: // NKikimr::NTxProxy::TResultStatus::ProxyNotReady
            case 20: // NKikimr::NTxProxy::TResultStatus::ProxyShardNotAvailable
            case 21: // NKikimr::NTxProxy::TResultStatus::ProxyShardTryLater
            case 22: // NKikimr::NTxProxy::TResultStatus::ProxyShardOverloaded
            case 51: // NKikimr::NTxProxy::TResultStatus::ExecTimeout:
            case 55: // NKikimr::NTxProxy::TResultStatus::ExecResultUnavailable:
                return EErrorKind::ErrorRetriable;
        }
    }

    if (FACILITY_FROM_CODE(code) == FACILITY_FILESTORE) {
        return EErrorKind::ErrorSilent;
    }

    // any other errors should not be retried automatically
    return EErrorKind::ErrorFatal;
}

bool IsConnectionError(const NProto::TError& e)
{
    return e.GetCode() == E_GRPC_UNAVAILABLE;
}

bool IsRetriable(const NProto::TError& e)
{
    return GetErrorKind(e) == EErrorKind::ErrorRetriable;
}

TString FormatError(const NProto::TError& e)
{
    TStringStream out;
    FormatResultCodePretty(out, e.GetCode());

    if (const auto& s = e.GetMessage()) {
        out << " " << s;
    }
    auto flags = e.GetFlags();
    ui32 flag = 1;
    while (flags) {
        if (flags & 1) {
            switch (flag) {
                case NProto::EF_SILENT: {
                    out << " f@silent";
                    break;
                }

                default: {}
            }
        }

        flags >>= 1;
        ++flag;
    }
    return out.Str();
}

TString FormatResultCode(ui32 code)
{
    TStringStream out;
    FormatResultCodePretty(out, code);

    return out.Str();
}

NProto::TError MakeError(ui32 code, TString message, ui32 flags)
{
    NProto::TError error;
    error.SetCode(code);
    error.SetFlags(flags);

    if (message) {
        error.SetMessage(std::move(message));
    }

    return error;
}

}   // namespace NCloud

////////////////////////////////////////////////////////////////////////////////

template <>
void Out<NCloud::TServiceError>(
    IOutputStream& out,
    const NCloud::TServiceError& e)
{
    NCloud::FormatResultCodePretty(out, e.GetCode());

    if (const auto& s = e.GetMessage()) {
        out << " " << s;
    }
}

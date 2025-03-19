#include "json.h"

#include <kernel/common_server/library/logging/events.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/regex/pcre/regexp.h>

#include <util/digest/fnv.h>
#include <util/string/type.h>
#include <util/stream/buffer.h>
#include <util/system/hostname.h>

namespace {
    constexpr TStringBuf DefaultReportLable = "";
    constexpr TStringBuf EventLogField = "__event_log";
    constexpr TStringBuf HostField = "__host";

    bool IsJson(TStringBuf s) {
        NJson::TJsonValue discarded;
        return NJson::ReadJsonFastTree(s, &discarded);
    }
}

TJsonReport::TJsonReport::TGuard::TGuard(IFrontendReportBuilder::TPtr report, int code)
    : TGuardBase(report, code)
    , JsonReport(MutableReportAs<TJsonReport>())
{
}

void TJsonReport::AddReportElement(TStringBuf name, NJson::TJsonValue&& elem, const bool allowUndefined) {
    if (!allowUndefined && !elem.IsDefined()) {
        return;
    }
    auto guard = MakeThreadSafeGuard();
    Report.SetValueByPath(name, std::move(elem));
}

void TJsonReport::AddReportElementString(TStringBuf name, TString&& elem) {
    Y_ASSERT(IsJson(elem));
    auto guard = MakeThreadSafeGuard();
    StrReports.emplace(name, std::move(elem));
}

void TJsonReport::SetExternalReportString(TString&& report, const bool flagIsJson) {
    Y_ASSERT(!flagIsJson || IsJson(report));
    auto guard = MakeThreadSafeGuard();
    StrReports.emplace(DefaultReportLable, std::move(report));
}

void TJsonReport::SetExternalReport(NJson::TJsonValue&& report) {
    auto guard = MakeThreadSafeGuard();
    CHECK_WITH_LOG(Report.IsNull() || (Report.IsMap() && report.IsMap()));
    if (Report.IsNull()) {
        Report = std::move(report);
    } else {
        for (auto&& [key, value] : report.GetMapSafe()) {
            Report.InsertValue(key, std::move(value));
        }
    }
}

bool TJsonReport::PrintReport(IOutputStream& so) const {
    auto guard = MakeThreadSafeGuard();
    auto defaultReport = StrReports.find(DefaultReportLable);
    if (defaultReport != StrReports.end()) {
//        Y_ASSERT(IsJson(defaultReport->second));
        so << defaultReport->second;
        return true;
    }

    const NJson::TJsonValue::TMapType* mapInfo = nullptr;
    if (Report.IsDefined()) {
        if (!Report.GetMapPointer(&mapInfo)) {
            return false;
        }
    }

    NJson::TJsonWriter writer(&so, false, ShouldSortKeys());
    writer.OpenMap();
    if (mapInfo) {
        for (auto&& [key, value] : *mapInfo) {
            writer.Write(key, value);
        }
    }
    for (auto&& [key, value] : StrReports) {
//        Y_ASSERT(IsJson(value));
        writer.UnsafeWrite(key, value);
    }
    writer.CloseMap();
    return true;
}

void TJsonReport::DoAddEvent(TInstant timestamp, const NCS::NLogging::TBaseLogRecord& e) {
    if (FirstEvent == TInstant::Zero()) {
        FirstEvent = timestamp;
    }
    NCS::NLogging::TLogRecordOperator<NCS::NLogging::TBaseLogRecord> eLocal(e);
    if (LastEvent != TInstant::Zero()) {
        eLocal("dt", Sprintf("%0.3f", 0.001 * (timestamp - LastEvent).MicroSeconds()));
    }
    eLocal("_ts", timestamp.MicroSeconds())("t", Sprintf("%0.3f", 0.001 * (timestamp - FirstEvent).MicroSeconds()));
    EventLog.emplace_back(std::move(eLocal));
    LastEvent = timestamp;
}

void TJsonReport::DoSetError(const TString& info) {
    Report.InsertValue("error", info);
}

NJson::TJsonValue TJsonReport::BuildJsonEventLog() const {
    NJson::TJsonValue jsonEvents = NJson::JSON_ARRAY;
    for (auto&& i : EventLog) {
        jsonEvents.AppendValue(i.SerializeToJson());
    }
    return jsonEvents;
}

void TJsonReport::DoFinish(const TCodedException& e) {
    if (EventLog.size()) {
        Report.InsertValue(EventLogField, BuildJsonEventLog());
        Report[HostField] = HostName();
    }
    if (e.GetCode() != HTTP_OK && e.HasReport()) {
        AddReportElement("code", e.GetErrorCode());
        AddReportElement("message", e.GetErrorMessage());
        AddReportElement("details", e.GetDetailedReport());
    }

    TBuffer report;
    if (!Report.IsNull() || StrReports.size()) {
        TBufferOutput output(report);
        if (!PrintReport(output)) {
            if (ShouldSortKeys()) {
                NJson::WriteJson(&output, &Report, /*formatOutput=*/false, /*sortKeys=*/true);
            } else {
                output << Report.GetStringRobust();
            }
        }
    }
    TStringBuf serialized = { report.data(), report.size() ? (report.size() - 1) : 0 };

    TString accessControlAllowOrigin;
    if (AccessControlAllowOrigin) {
        TString origin{Context->GetBaseRequestData().HeaderInOrEmpty("Origin")};
        if (AccessControlAllowOrigin->Match(origin.c_str())) {
            accessControlAllowOrigin = origin;
        }
    } else {
        accessControlAllowOrigin = "*";
    }
    if (accessControlAllowOrigin) {
        Context->AddReplyInfo("Access-Control-Allow-Origin", accessControlAllowOrigin, false);
    }
    const TString& reqid = Context->GetCgiParameters().Get("reqid");
    if (reqid) {
        Context->AddReplyInfo("X-YaRequestId", reqid, false);
    }
    Context->AddReplyInfo("Content-Type", OverridenContentType ? OverridenContentType : "application/json", false);
    Context->MakeSimpleReply(report, e.GetCode());

    {
        const auto totalTime = Now() - Context->GetRequestStartTime();
        NJson::TJsonValue replyHeaders;
        for (auto&& [headerName, headerValue] : Context->GetReplyHeaders()) {
            replyHeaders[headerName] = headerValue;
        }

        auto eventRecord = TFLEventLog::ModuleLog("response")
                           ("_type", "response")
                           ("meta_type", TString(Context->GetUri()))
                           ("meta_code", e.GetCode())
                           ("total_time", totalTime.SecondsFloat())
                           ("error", (e.GetCode() >= 500) ? "true" : "false")
                           ("http_code", e.GetCode())
                           ("reqid", reqid)
                           ("link", Context->GetLink())
                           ("trace_id", Context->GetTraceId())
                           ("response_headers", replyHeaders.GetStringRobust());
        for (auto [k, v] : EventCodes) {
            eventRecord(k, v);
        }
        eventRecord("body", Obfuscator.Obfuscate(TString(serialized)), LogLineLimit);
    }
}

bool TJsonReport::ShouldSortKeys() const {
    return SortKeys || EventLog.size();
}

#include "serverstat.h"
#include "server.h"

#include <library/cpp/unistat/unistat.h>

#include <util/generic/algorithm.h>

namespace {
    TInstanceStats ExportToProto(int level) {
        TInstanceStats stats;
        TUnistat::Instance().ExportToProto(stats, level);
        return stats;
    }

    void DumpJson(IOutputStream& out, int level) {
        out << TUnistat::Instance().CreateJsonDump(level, false);
    }

    void DumpProto(IOutputStream& out, int level) {
        ExportToProto(level).SerializeToArcadiaStream(&out);
    }

    void DumpProtoHr(IOutputStream& out, int level) {
        out << ExportToProto(level).Utf8DebugString();
    }

    void DumpProtoJson(IOutputStream& out, int level) {
        ExportToProto(level).PrintJSON(out);
    }
}

void TServerStats::TIniter::Init(TUnistat& unistat) const {
    // TODO: generate intervals automatically
    const TVector<double> INTERVALS = {
            0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
            10, 20, 30, 40, 50, 60, 70, 80, 90,
            100, 200, 300, 400, 500,
            1000, 2000, 5000,
            10000, 20000
    };

    Stats.Timings = unistat.DrillHistogramHole(TString::Join(Prefix, "answer_time"), TString("dhhh"), NUnistat::TPriority(0), Intervals);
    Stats.ProductionTimings = unistat.DrillHistogramHole(TString::Join(Prefix, "production_answer_time"), TString("dhhh"), NUnistat::TPriority(0), Intervals);
    Stats.Code200 = unistat.DrillFloatHole(TString::Join(Prefix, "codes_200"), TString("dmmv"), NUnistat::TPriority(0));
    Stats.Code404 = unistat.DrillFloatHole(TString::Join(Prefix, "codes_404"), TString("dmmv"), NUnistat::TPriority(0));
    Stats.Code304 = unistat.DrillFloatHole(TString::Join(Prefix, "codes_304"), TString("dmmv"), NUnistat::TPriority(0));
    Stats.Code5xx = unistat.DrillFloatHole(TString::Join(Prefix, "codes_5xx"), TString("dmmv"), NUnistat::TPriority(0));
    Stats.CodeOther = unistat.DrillFloatHole(TString::Join(Prefix, "codes_other"), TString("dmmv"), NUnistat::TPriority(0));

    Stats.RequestQueueSize = unistat.DrillHistogramHole(
        TString::Join(Prefix, "request_queue_size"), "dhhh",
        NUnistat::TPriority(0),
        INTERVALS
    );
    Stats.RequestQueueObjectCount = unistat.DrillHistogramHole(
        TString::Join(Prefix, "request_queue_object_count"), "dhhh",
        NUnistat::TPriority(0),
        INTERVALS
    );
    Stats.RequestQueueReject = unistat.DrillFloatHole(
        TString::Join(Prefix, "request_queue_reject"), "dmmm",
        NUnistat::TPriority(0),
        NUnistat::TStartValue(0),
        EAggregationType::Sum,
        true
    );
    Stats.FailQueueSize = unistat.DrillHistogramHole(
        TString::Join(Prefix, "fail_queue_size"), "dhhh",
        NUnistat::TPriority(0),
        INTERVALS
    );
    Stats.FailQueueObjectCount = unistat.DrillHistogramHole(
        TString::Join(Prefix, "fail_queue_object_count"), "dhhh",
        NUnistat::TPriority(0),
        INTERVALS
    );
    Stats.FailQueueReject = unistat.DrillFloatHole(
        TString::Join(Prefix, "fail_queue_reject"), "dmmm",
        NUnistat::TPriority(0),
        NUnistat::TStartValue(0),
        EAggregationType::Sum,
        true
    );
}

void TServerStats::Init(TString prefix, TVector<double> intervals) {
    intervals.push_back(0.0);
    Sort(intervals.begin(), intervals.end());
    intervals.erase(Unique(intervals.begin(), intervals.end()), intervals.end());
    Intervals = std::move(intervals);

    TIniter initer(*this, std::move(prefix), Intervals);
    initer.Init(TUnistat::Instance());
}

void TServerStats::HitTime(double time, bool productionTraffic) {
    if (Y_LIKELY(!!Timings)) {
        Timings->PushSignal(time);
    }
    if (productionTraffic && Y_LIKELY(!!ProductionTimings)) {
        ProductionTimings->PushSignal(time);
    }
}

void TServerStats::HitRequest(HttpCodes code) {
    NUnistat::IHolePtr hole = nullptr;
    switch (ui32(code)) {
        case 200:
            hole = Code200;
            break;
        case 404:
            hole = Code404;
            break;
        case 304:
            hole = Code304;
            break;
        default:
            if (ui32(code) / 100 == 5) {
                hole = Code5xx;
            } else {
                hole = CodeOther;
            }
            break;
    }

    if (Y_LIKELY(!!hole)) {
        hole->PushSignal(1.0);
    }
}

void TServerStats::HitQueue(
    size_t requestQueueSize, size_t failQueueSize,
    TMaybe<size_t> requestQueueObjectCount, TMaybe<size_t> failQueueObjectCount) {
    if (Y_LIKELY(!!RequestQueueSize)) {
        RequestQueueSize->PushSignal(requestQueueSize);
    }
    if (Y_LIKELY(!!RequestQueueObjectCount) && requestQueueObjectCount) {
        RequestQueueObjectCount->PushSignal(*requestQueueObjectCount);
    }
    if (Y_LIKELY(!!FailQueueSize)) {
        FailQueueSize->PushSignal(failQueueSize);
    }
    if (Y_LIKELY(!!FailQueueObjectCount) && failQueueObjectCount) {
        FailQueueObjectCount->PushSignal(*failQueueObjectCount);
    }
}

void TServerStats::HitFailQueueReject() {
    if (Y_LIKELY(!!FailQueueReject)) {
        FailQueueReject->PushSignal(1.0);
    }
}

void TServerStats::HitRequestQueueReject() {
    if (Y_LIKELY(!!RequestQueueReject)) {
        RequestQueueReject->PushSignal(1.0);
    }
}

void TServerStats::ReportMonitoringStat(IOutputStream& out, TServerStats::EReportFormat fmt, int level) const {
    switch (fmt) {
        case EReportFormat::Json:
            DumpJson(out, level);
            break;
        case EReportFormat::Proto:
            DumpProto(out, level);
            break;
        case EReportFormat::ProtoHr:
            DumpProtoHr(out, level);
            break;
        case EReportFormat::ProtoJson:
            DumpProtoJson(out, level);
            break;
        default:
            DumpJson(out, level);
            break;
    }
}

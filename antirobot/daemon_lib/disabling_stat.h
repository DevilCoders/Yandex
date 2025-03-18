#pragma once

#include "stat.h"

#include <antirobot/lib/stats_writer.h>


namespace NAntiRobot {


class TDisablingStat {
public:
    enum class ECounter {
        NotBannedRequests            /* "not_banned_requests" */,
        NotBlockedRequests           /* "not_blocked_requests" */,
        StoppedFuryPreprodRequests  /* "stopped_fury_preprod_requests" */,
        StoppedFuryRequests         /* "stopped_fury_requests" */,
        Count
    };

    TCategorizedStats<std::atomic<size_t>, ECounter> Counters;

    enum class EExpBinCounter {
        NotBannedRequestsCbb        /* "not_banned_requests_cbb" */,
        Count
    };

    TCategorizedStats<std::atomic<size_t>, EExpBinCounter, EExpBin> ExpBinCounters;

    enum class EHistogram {
        PanicCanShowCaptchaStatus    /* "panic_canshowcaptcha_status" */,
        PanicCbbStatus               /* "panic_cbb_status" */,
        PanicMainOnlyStatus          /* "panic_morda_status" */,
        PanicDzensearchStatus        /* "panic_dzensearch_status" */,
        PanicMayBanForStatus         /* "panic_maybanfor_status" */,
        PanicNeverBanStatus          /* "panic_neverban_status" */,
        PanicNeverBlockStatus        /* "panic_neverblock_status" */,
        PanicPreviewIdentTypeStatus  /* "panic_preview_ident_type_status" */,
        Count
    };

    TCategorizedHistograms<
        std::atomic<size_t>, EHistogram,
        EHostType
    > Histograms;

    void PrintStats(TStatsWriter& out) const {
        Counters.Print(out);
        ExpBinCounters.Print(out);
        Histograms.Print(out);
    }

    void AddNotBlockedRequest() {
        Counters.Inc(ECounter::NotBlockedRequests);
    }

    void AddNotBannedRequest() {
        Counters.Inc(ECounter::NotBannedRequests);
    }

    void AddNotBannedRequestCbb(EExpBin expBin) {
        ExpBinCounters.Inc(expBin, EExpBinCounter::NotBannedRequestsCbb);
    }

    void AddStoppedFuryRequest() {
        Counters.Inc(ECounter::StoppedFuryRequests);
    }

    void AddStoppedFuryPreprodRequest() {
        Counters.Inc(ECounter::StoppedFuryPreprodRequests);
    }

    void EnablePanicMayBanFor(EHostType service) {
        Histograms.Get(service, EHistogram::PanicMayBanForStatus) = 1;
    }

    void DisablePanicMayBanFor(EHostType service) {
        Histograms.Get(service, EHistogram::PanicMayBanForStatus) = 0;
    }

    void EnablePanicCanShowCaptcha(EHostType service) {
        Histograms.Get(service, EHistogram::PanicCanShowCaptchaStatus) = 1;
    }

    void DisablePanicCanShowCaptcha(EHostType service) {
        Histograms.Get(service, EHistogram::PanicCanShowCaptchaStatus) = 0;
    }

    void EnablePanicPreviewIdentTypeEnabled(EHostType service) {
        Histograms.Get(service, EHistogram::PanicPreviewIdentTypeStatus) = 1;
    }

    void DisablePanicPreviewIdentTypeEnabled(EHostType service) {
        Histograms.Get(service, EHistogram::PanicPreviewIdentTypeStatus) = 0;
    }

    void EnablePanicCbb(EHostType service) {
        Histograms.Get(service, EHistogram::PanicCbbStatus) = 1;
    }

    void DisablePanicCbb(EHostType service) {
        Histograms.Get(service, EHistogram::PanicCbbStatus) = 0;
    }

    void EnablePanicMainOnly(EHostType service) {
        Histograms.Get(service, EHistogram::PanicMainOnlyStatus) = 1;
    }

    void DisablePanicMainOnly(EHostType service) {
        Histograms.Get(service, EHistogram::PanicMainOnlyStatus) = 0;
    }

    void EnablePanicDzensearchOnly(EHostType service) {
        Histograms.Get(service, EHistogram::PanicDzensearchStatus) = 1;
    }

    void DisablePanicDzensearchOnly(EHostType service) {
        Histograms.Get(service, EHistogram::PanicDzensearchStatus) = 0;
    }

    void EnablePanicNeverBan(EHostType service) {
        Histograms.Get(service, EHistogram::PanicNeverBanStatus) = 1;
    }

    void DisablePanicNeverBan(EHostType service) {
        Histograms.Get(service, EHistogram::PanicNeverBanStatus) = 0;
    }

    void EnablePanicNeverBlock(EHostType service) {
        Histograms.Get(service, EHistogram::PanicNeverBlockStatus) = 1;
    }

    void DisablePanicNeverBlock(EHostType service) {
        Histograms.Get(service, EHistogram::PanicNeverBlockStatus) = 0;
    }
};


} // namespace NAntiRobot

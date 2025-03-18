#include "http_common.h"

#include <util/string/join.h>
#include <util/string/util.h>

void UpdateStatsByTasks(TStateStats* stats, const TMasterTarget::TConstTaskIterator& tasks) {
    TMasterTarget::TConstTaskIterator copy = tasks;
    while (copy.Next()) {
        stats->Update(copy->Status);
    }
}

void FormatDuration(IOutputStream& out, int delta) {
    if (delta < 0) {
        out << "-";
    } else {
        out << Sprintf("%d:%02d:%02d", delta / 3600, (delta / 60) % 60, delta % 60);
    }
}

TStateFilter::TStateFilter()
    : EnabledPositive(false)
    , EnabledNegative(false)
{
}

void TStateFilter::Enable(const TString& what) {
    try {
        WantState = FromString<TTaskState>(what);
    } catch (const yexception& /*e*/) {
        return;
    }

    EnabledPositive = true;
    WantStateStr = what;
}

void TStateFilter::EnableNegative(const TString& what) {
    try {
        DontWantState = FromString<TTaskState>(what);
    } catch (const yexception& /*e*/) {
        return;
    }

    EnabledNegative = true;
    DontWantStateStr = what;
}

TString TStateFilter::GetDescr() {
    if (EnabledPositive && EnabledNegative) {
        return TString(" (showing having " + WantStateStr + " but don't having " + DontWantStateStr + " state)");
    } else if (EnabledPositive) {
        return TString(" (showing having " + WantStateStr + " state)");
    } else if (EnabledNegative) {
        return TString(" (showing don't having " + DontWantStateStr + " state)");
    } else {
        return TString();
    }
}

bool TStateFilter::ShouldDrop(const TTaskStatus& status) const {
    const TTaskState& gotState = status.GetState();
    return EnabledPositive && (gotState != WantState)
            || EnabledNegative && (gotState == DontWantState);
}

bool TStateFilter::ShouldDrop(const TStateStats& stats) const {
    return EnabledPositive && (stats.Counter.GetCount(WantState) == 0)
            || EnabledNegative && (stats.Counter.GetCount(DontWantState) > 0);
}


TString NStatsFormat::FormatString(const TStateStats& stats) {
    TVector<TStringBuf> states;
    for (auto stateInfo = TStateRegistry::begin(); stateInfo != TStateRegistry::end(); ++stateInfo) {
        if (stats.Counter.GetCount(stateInfo->State) > 0) {
            states.emplace_back(stateInfo->SmallName);
        }
    }
    return JoinSeq(",", states);
}

void NStatsFormat::FormatText(const TStateStats& stats, IOutputStream& out) {
    out << NStatsFormat::FormatString(stats);
}

void NStatsFormat::FormatXML(const TStateStats& stats, IOutputStream& out) {
    for (TStateRegistry::const_iterator i = TStateRegistry::begin(); i != TStateRegistry::end(); ++i) {
        int n;
        if ((n = stats.Counter.GetCount(i->State)) > 0)
            out << "<" << i->XmlTag << ">" << n << "</" << i->XmlTag << ">";
    }
}

void NStatsFormat::FormatJSON(const TStateStats& stats, IOutputStream& out, bool all) {
    out << "{\"t\":" << stats.Counter.GetTotalCount();
    for (TStateRegistry::const_iterator i = TStateRegistry::begin(); i != TStateRegistry::end(); ++i) {
        int n;
        if ((n = stats.Counter.GetCount(i->State)) > 0 || all)
            out << ",\"" << i->JsonId << "\":" << n;
    }
    out << "}";
}

void NStatsFormat::FormatText(const TTimeStats& time, IOutputStream& out) {
    out << time.LastStarted << " " << time.LastFinished << " " << time.LastSuccess << " " << time.LastFailure;
}

void NStatsFormat::FormatXML(const TTimeStats& time,IOutputStream& out) {
    if (time.LastStarted)
        out << "<laststarted>" << time.LastStarted << "</laststarted>";

    if (time.LastFinished)
        out << "<lastfinished>" << time.LastFinished << "</lastfinished>";

    if (time.LastSuccess)
        out << "<lastsuccess>" << time.LastSuccess << "</lastsuccess>";

    if (time.LastFailure)
        out << "<lastfailure>" << time.LastFailure << "</lastfailure>";
}

void NStatsFormat::FormatJSON(const TTimeStats& time, IOutputStream& out) {
    out << "\"s\":" << time.LastStarted << ",\"f\":" << time.LastFinished << ",\"S\":" << time.LastSuccess << ",\"F\":" << time.LastFailure;
}

void NStatsFormat::FormatText(const TResStats& res, IOutputStream& out) {
    out << res.CpuMax << " " << res.CpuAvg << " " << res.MemMax << " " << res.MemAvg;
}

void NStatsFormat::FormatXML(const TResStats& res, IOutputStream& out) {
    if (res.CpuMax)
        out << "<cpuMax>" << res.CpuMax << "</cpuMax>";

    if (res.CpuAvg)
        out << "<cpuAvg>" << res.CpuAvg << "</cpuAvg>";

    if (res.MemMax)
        out << "<memMax>" << res.MemMax << "</memMax>";

    if (res.MemAvg)
        out << "<memAvg>" << res.MemAvg << "</memAvg>";
}

void NStatsFormat::FormatJSON(const TResStats& res, IOutputStream& out) {
    out << "\"cM\":" << res.CpuMax << ",\"ca\":" << res.CpuAvg << ",\"mM\":" << res.MemMax << ",\"ma\":" << res.MemAvg;
}

void NStatsFormat::FormatText(const TRepeatStats& repeat, IOutputStream& out) {
    out << repeat.repeatedTimes << " " << repeat.maxRepeatTimes;
}

void NStatsFormat::FormatXML(const TRepeatStats& repeat, IOutputStream& out) {
    if (repeat.repeatedTimes)
        out << "<repeatedTimes>" << repeat.repeatedTimes << "</repeatedTimes>";

    if (repeat.maxRepeatTimes)
        out << "<maxRepeatTimes>" << repeat.maxRepeatTimes << "</maxRepeatTimes>";
}

void NStatsFormat::FormatJSON(const TRepeatStats& repeat, IOutputStream& out) {
    out << "{\"REPC\":" << repeat.repeatedTimes << ",\"REPM\":" << repeat.maxRepeatTimes << "}";
}

void NStatsFormat::FormatText(const TDurationStats& duration, IOutputStream& out) {
    FormatDuration(out, duration.Minimum);
    out << " ";
    FormatDuration(out, duration.Average);
    out << " ";
    FormatDuration(out, duration.Maximum);
}

void NStatsFormat::FormatXML(const TDurationStats& duration, IOutputStream& out) {
    out << "<duration>";
    out << "<min>" << duration.Minimum << "</min>";
    out << "<avg>" << duration.Average << "</avg>";
    out << "<max>" << duration.Maximum << "</max>";
    out << "</duration>";
}

void NStatsFormat::FormatJSON(const TDurationStats& duration, IOutputStream& out) {
    out << "\"dm\":" << duration.Minimum << ",\"da\":" << duration.Average << ",\"dM\":" << duration.Maximum;
}

void NStatsFormat::FormatText(const TAggregateDurationStats& duration, IOutputStream& out) {
    FormatDuration(out, duration.Actual);
    out << " ";
    FormatDuration(out, duration.Total);
}

void NStatsFormat::FormatJSON(const TAggregateDurationStats& duration, IOutputStream& out) {
    out << "\"dA\":" << duration.Actual << ",\"dt\":" << duration.Total;
}

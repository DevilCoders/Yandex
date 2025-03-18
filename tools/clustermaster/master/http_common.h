#pragma once

#include "master_target.h"
#include "master_target_graph.h"
#include "state_registry.h"

#include <util/stream/file.h>

#include <time.h>

void UpdateStatsByTasks(TStateStats* stats, const TMasterTarget::TConstTaskIterator& tasks);

namespace NStatsFormat {
    TString FormatString(const TStateStats& state);
    void FormatText(const TStateStats& state, IOutputStream& out);
    void FormatXML(const TStateStats& state, IOutputStream& out);
    void FormatJSON(const TStateStats& state, IOutputStream& out, bool all = false);

    void FormatText(const TTimeStats& time, IOutputStream& out);
    void FormatXML(const TTimeStats& time,IOutputStream& out);
    void FormatJSON(const TTimeStats& time, IOutputStream& out);

    void FormatText(const TResStats& res, IOutputStream& out);
    void FormatXML(const TResStats& res, IOutputStream& out);
    void FormatJSON(const TResStats& res, IOutputStream& out);

    void FormatText(const TRepeatStats& repeat, IOutputStream& out);
    void FormatXML(const TRepeatStats& repeat, IOutputStream& out);
    void FormatJSON(const TRepeatStats& repeat, IOutputStream& out);

    void FormatText(const TDurationStats& duration, IOutputStream& out);
    void FormatXML(const TDurationStats& duration, IOutputStream& out);
    void FormatJSON(const TDurationStats& duration, IOutputStream& out);

    void FormatText(const TAggregateDurationStats& duration, IOutputStream& out);
    void FormatJSON(const TAggregateDurationStats& duration, IOutputStream& out);
}

struct TStateFilter {
    TStateFilter();

    void Enable(const TString& what);
    void EnableNegative(const TString& what);
    bool ShouldDrop(const TTaskStatus& status) const;
    bool ShouldDrop(const TStateStats& stats) const;

    bool IsEnabled() { return EnabledPositive || EnabledNegative; }
    TString GetDescr();

    bool EnabledPositive;
    TTaskState WantState;
    TString WantStateStr;
    bool EnabledNegative;
    TTaskState DontWantState;
    TString DontWantStateStr;
};

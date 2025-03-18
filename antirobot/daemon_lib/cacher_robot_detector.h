#pragma once

#include <antirobot/lib/stats_writer.h>

#include <util/generic/fwd.h>
#include <util/generic/noncopyable.h>
#include <util/generic/yexception.h>
#include "request_context.h"

#include "stat.h"
#include "time_stats.h"

namespace NAntiRobot {

class TRequest;
struct TEnv;
struct TRawCacherFactors;
struct TRobotStatus;

enum class ECacherRobotDetectorCounter {
    Whitelisted  /* "cacher_request_whitelisted" */,
    Count
};

class TCacherRobotDetector {
public:
    TCacherRobotDetector();
    ~TCacherRobotDetector();
    std::tuple<bool, float> Process(const TRequest& req, TRequestContext& rc, TRobotStatus& robotStatus);
    void PrintStats(TStatsWriter& out) const;
    void PrintStatsLW(TStatsWriter& out, EStatType service) const;
    TTimeStats& GetFactorsCalcTimeStats();
private:
    class TImpl;
    THolder<TImpl> Impl;
};

} //namespace NAntiRobot

#include "cacher_robot_detector.h"

#include "cacher_factors.h"
#include "classificator.h"
#include "config_global.h"
#include "environment.h"
#include "eventlog_err.h"
#include "req_types.h"
#include "request_params.h"
#include "robot_detector.h"
#include "stat.h"
#include "unified_agent_log.h"

#include <antirobot/lib/stats_writer.h>

#include <library/cpp/iterator/enumerate.h>

#include <util/generic/ptr.h>
#include <util/generic/xrange.h>

#include <array>


namespace NAntiRobot {

namespace {

float GetCacherThreshold(const TRequest& req) {
    return ANTIROBOT_DAEMON_CONFIG.JsonConfig.ParamsByTld(req.HostType, req.Tld).CacherThreshold;
}


class TZoneData : public TNonCopyable {
    std::array<TSimpleSharedPtr<TClassificator<TCacherLinearizedFactors>>, HOST_NUMTYPES> Classifies;

public:
    explicit TZoneData(const std::array<TString, HOST_NUMTYPES>& formulaFiles) {
        for (auto i : xrange(static_cast<size_t>(HOST_NUMTYPES))) {
            const auto hostType = static_cast<EHostType>(i);
            Classifies[hostType].Reset(CreateCacherClassificator(formulaFiles[i]));
        }
    }

    const TClassificator<TCacherLinearizedFactors>& GetClassifyForHost(EHostType hostType) const {
        return *Classifies[hostType].Get();
    }
};

using TZoneDataMap = THashMap<TCiString, TAtomicSharedPtr<TZoneData>>;

class TStats {
public:
    TStats()
        : FactorsCalcTimeStats(TIME_STATS_VEC_CACHER, "cacher_request_features_")
        , FormulaCalcTimeStats(TIME_STATS_VEC_CACHER, "cacher_formula_calc_")
    {
    }

    void Print(TStatsWriter& out) const {
        FactorsCalcTimeStats.PrintStats(out);
        FormulaCalcTimeStats.PrintStats(out);
        Counters.Print(out);
    }

    void PrintLW(TStatsWriter& out, EStatType service) const {
        const auto host = StatTypeToHostType(service);
        if (service != EStatType::STAT_TOTAL) {
            size_t totalCounter = 0;
            for (size_t expBinIdx = 0; expBinIdx < EnumValue(EExpBin::Count); ++expBinIdx) {
                const auto expBin = static_cast<EExpBin>(expBinIdx);
                totalCounter += Counters.Get(
                        host, expBin,
                        ECacherRobotDetectorCounter::Whitelisted
                        );
            }

            out.WriteScalar(
                    ToString(ECacherRobotDetectorCounter::Whitelisted),
                    totalCounter
                    );
        }
    }

    TTimeStats& GetFactorsCalcTimeStats() {
        return FactorsCalcTimeStats;
    }

    TTimeStats& GetFormulaCalcTimeStats() {
        return FormulaCalcTimeStats;
    }

    void Whitelisted(const TRequest& req) {
        Counters.Inc(req, ECacherRobotDetectorCounter::Whitelisted);
    }

private:
    TTimeStats FactorsCalcTimeStats;
    TTimeStats FormulaCalcTimeStats;
    TCategorizedStats<
        std::atomic<size_t>, ECacherRobotDetectorCounter,
        EHostType, EExpBin
    > Counters;
};

} // anonymous namespace

class TCacherRobotDetector::TImpl {
public:
    explicit TImpl() {
        TAntirobotDaemonConfig::TZoneConfNames confNames = ANTIROBOT_DAEMON_CONFIG.GetZoneConfNames();

        for (const auto& tld: confNames) {
            ZoneData[tld] = new TZoneData(ANTIROBOT_DAEMON_CONFIG.JsonConfig.GetCacherFormulaFilesArrayByTld(tld));
        }
    }

    ~TImpl() = default;

    std::tuple<bool, float> Process(const TRequest& req, TRequestContext& rc, TRobotStatus& robotStatus) {
        bool whitelisted = false;
        float catboostResult = 0.0F;

        if ((!req.CanShowCaptcha()
                || req.InitiallyWasXmlsearch)
                && req.HostType != HOST_MARKETFAPI_BLUE
                && req.HostType != HOST_MARKETAPI) {
            return {whitelisted, catboostResult};
        }

        try {
            const bool alreadyRobot = robotStatus.IsCaptcha();

            const float threshold = GetCacherThreshold(req);
            catboostResult = GetCatboostResult(req, rc.CacherFactors);

            const bool canBeBanned = !req.UserAddr.IsWhitelisted() && req.MayBanFor() && req.Uid.Ns != TUid::SPRAVKA;

            // баним на автору
            if (canBeBanned && !alreadyRobot && catboostResult >= threshold && req.HostType == HOST_AUTORU) {
                rc.BlockReason = "CACHER_FORMULA";
                if (req.CanShowCaptcha()) {
                    robotStatus.RegexpCaptcha = true;
                    rc.CatboostBan = true;
                }
            }

            // обеляем на вебе и ident_type == 1, 8
            if (alreadyRobot && robotStatus.RobotsetCaptcha && catboostResult >= threshold && req.Uid.IpBased() && req.HostType == HOST_WEB) {
                // обеляем пользователя:
                robotStatus.Reset();
                whitelisted = true;
                Stats.Whitelisted(req);
            }

            if (alreadyRobot || RandomNumber<double>() < ANTIROBOT_DAEMON_CONFIG.CacherRandomFactorsProbability) {
                NAntirobotEvClass::TCacherFactors ev;
                ev.MutableHeader()->CopyFrom(req.MakeLogHeader());

                FillCacherFactors(ev, *rc.CacherFeatures, catboostResult, threshold);

                NAntirobotEvClass::TProtoseqRecord rec = CreateEventLogRecord(ev);
                ANTIROBOT_DAEMON_CONFIG.EventLog->WriteLogRecord(rec);
            }
        } catch (...) {
            EVLOG_MSG << EVLOG_ERROR << req << "CacherRequestFactors failure " << CurrentExceptionMessage();
        }
        return {whitelisted, catboostResult};
    }

    void PrintStats(TStatsWriter& out) const {
        Stats.Print(out);
    }

    void PrintStatsLW(TStatsWriter& out, EStatType service) const {
        Stats.PrintLW(out, service);
    }

    float GetCatboostResult(const TRequest& req, const TRawCacherFactors& cacherFactors) {
        const auto& zoneData = GetZoneData(req.Tld);
        const auto& classify = zoneData.GetClassifyForHost(req.HostType);

        TMeasureDuration measureDuration{Stats.GetFormulaCalcTimeStats()};
        return classify(cacherFactors.Factors);
    }

    TTimeStats& GetFactorsCalcTimeStats() {
        return Stats.GetFactorsCalcTimeStats();
    }

private:
    inline TZoneData& GetZoneData(const TStringBuf& tld) {
        return *ZoneData[ANTIROBOT_DAEMON_CONFIG.ConfByTld(tld).Tld];
    }

    TZoneDataMap ZoneData;
    TStats Stats;
};


TCacherRobotDetector::TCacherRobotDetector()
    : Impl(new TImpl)
{
}

TCacherRobotDetector::~TCacherRobotDetector() = default;

std::tuple<bool, float> TCacherRobotDetector::Process(const TRequest& req, TRequestContext& rc, TRobotStatus& robotStatus) {
    return Impl->Process(req, rc, robotStatus);
}

void TCacherRobotDetector::PrintStats(TStatsWriter& out) const {
    return Impl->PrintStats(out);
}

void TCacherRobotDetector::PrintStatsLW(TStatsWriter& out, EStatType service) const {
    return Impl->PrintStatsLW(out, service);
}

TTimeStats& TCacherRobotDetector::GetFactorsCalcTimeStats() {
    return Impl->GetFactorsCalcTimeStats();
}

} // namespace NAntiRobot

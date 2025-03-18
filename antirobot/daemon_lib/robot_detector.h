#pragma once

#include "blocker.h"
#include "cbb.h"
#include "stat.h"
#include "training_set_generator.h"

namespace NAntiRobot {
    struct TRequestContext;
    struct TRequestFeatures;
    struct TEnv;
    class TRequest;
    class TIsBlocked;

    enum class EMissedRobotStatus {
        NotMissed,
        MissedOnce,
        MissedMultiple,
    };

    enum class ERobotDetectorCounter {
        ProcessingsOnCacher  /* "processings_on_cacher" */,
        Count
    };

    enum class ERobotDetectorServiceCounter {
        MarkedByYql                /* "marked_by_yql" */,
        MissedMultipleRobots       /* "missed_multiple_robots" */,
        MissedOnceRobots           /* "missed_once_robots" */,
        RobotsWithCaptcha          /* "robots_with_captcha" */,
        UniqueRandomFactors        /* "unique_random_factors" */,
        WhitelistQueries           /* "whitelist_queries" */,
        WhitelistQueriesSusp       /* "whitelist_queries_susp" */,
        WizardFactors              /* "wizard_errors" */,
        Count
    };

    enum class ERobotDetectorServiceNsGroupCounter {
        QueriesTrustedUsers             /* "requests_trusted_users" */,
        QueriesWithCaptcha              /* "requests_with_captcha" */,
        QueriesWithCaptchaTrustedUsers  /* "requests_with_captcha_trusted_users" */,
        Robots                          /* "robots" */,
        Queries                         /* "requests_req" */,
        Count
    };

    enum class ERobotDetectorServiceNsExpBinCounter {
        Queries                         /* "requests" */,
        Count
    };

    struct TRobotStatus {
        bool ForceCaptcha = false;
        bool SuspiciousnessCaptcha = false;
        bool RobotsetCaptcha = false;
        bool RegexpCaptcha = false;

        bool IsCaptcha() const {
            return ForceCaptcha || SuspiciousnessCaptcha || RobotsetCaptcha || RegexpCaptcha;
        }

        void Reset() {
            ForceCaptcha = false;
            SuspiciousnessCaptcha = false;
            RobotsetCaptcha = false;
            RegexpCaptcha = false;
        }

        static TRobotStatus Normal() {
            return TRobotStatus{};
        }
    };

    struct TDetectedRobotInfo {
        bool IsRobot = false;
        bool IsForcedRobot = false;
    };

    TDetectedRobotInfo GetRobotInfo(
        const TRequestContext& rc,
        const TRequestFeatures& rf,
        bool matchedRules,
        bool matchedMatrixnet,
        bool matchedCbb,
        size_t requestsFromUid,
        bool isAlreadyRobot
    );

    class IRobotDetector {
    public:
        virtual ~IRobotDetector() {
        }

        virtual TRobotStatus GetRobotStatus(TRequestContext& rc) = 0;
        virtual void ProcessRequest(const TRequestContext& rc, bool onCacher = false) = 0;
        virtual void ProcessCaptchaInput(const TRequestContext& rc, bool correctInput) = 0;
        virtual void PrintStats(const TRequestGroupClassifier& groupClassifier, TStatsWriter& out) const = 0;
        virtual void PrintStatsLW(TStatsWriter& out, EStatType service) const = 0;
        virtual ITrainingSetGenerator& GetTrainSetGenerator(const TStringBuf& tld) = 0;
    };

    class TDummyRobotDetector : public IRobotDetector {
    public:
        void ProcessRequest(const TRequestContext&, bool) override {}
        void ProcessCaptchaInput(const TRequestContext&, bool) override {};
        void PrintStats(const TRequestGroupClassifier&, TStatsWriter&) const override {};
        void PrintStatsLW(TStatsWriter&, EStatType) const override {};
        ITrainingSetGenerator& GetTrainSetGenerator(const TStringBuf&) override {
            return TrainSetGen;
        }
        TRobotStatus GetRobotStatus(TRequestContext&) override {
            return TRobotStatus::Normal();
        };
    private:
        class TTrainingSetGeneratorAdapter : public ITrainingSetGenerator {
        public:
            EResult ProcessRequest(const TRequestContext&) override {
                return EResult::RequestNotUsed;
            };
            TStringBuf Name() override {
                return "";
            };
            void ProcessCaptchaGenerationError(const TRequestContext&) override {
            }
        };
        TTrainingSetGeneratorAdapter TrainSetGen;
    };

    IRobotDetector* CreateRobotDetector(const TEnv& env, const std::array<THolder<TIsBlocked>, EHostType::HOST_NUMTYPES>& isBlocked, TBlocker& blocker, ICbbIO* cbbIO);

    const size_t MAX_REQ_URL_CLCK_SIZE = 48;
    const size_t MAX_REFERER_URL_SIZE = 30;

    const TString TEST_REQUESTS_FOR_CAPTCHA[] = {
        "Капчу!",
        "e48a2b93de1740f48f6de0d45dc4192a",
    };

    const TString TEST_REQUEST_FOR_BLOCK = "a17c4f36e72208c19d13d46a1408945f";
    const TDuration FORCED_BLOCK_PERIOD = TDuration::Seconds(29);
}

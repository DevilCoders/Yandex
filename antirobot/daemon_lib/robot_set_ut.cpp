#include "robot_set.h"

#include <antirobot/daemon_lib/ut/utils.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/random/shuffle.h>
#include <util/stream/null.h>
#include <util/thread/pool.h>

#include <tuple>


namespace NAntiRobot {


THolder<TRequest> CreateRequest(const TUid& uid, TInstant arrivalTime, EHostType hostType) {
    const TString request = TString("GET /search?text=cats HTTP/1.1\r\n") +
                            "X-Start-Time: " + ToString(arrivalTime.MicroSeconds()) + "\r\n" +
                            "X-Antirobot-Service-Y: " + ToString(hostType) + "\r\n" +
                            "X-Forwarded-For-Y: 0.0.0.0\r\n"
                            "\r\n";

    auto ret = CreateDummyParsedRequest(request);
    ret->Uid = uid;

    return ret;
}


Y_UNIT_TEST_SUITE(RobotContainer) {
    Y_UNIT_TEST(AddLCookieIdentifiedRequest) {
        const auto robots = MakeHolder<TRobotSet>();

        TInstant arrivalTime = TInstant::Now();
        EHostType hostType = EHostType::HOST_WEB;
        auto uid = TUid::FromLCookie(NLCookie::TLCookie{});

        auto request = CreateRequest(uid, arrivalTime, hostType);
        robots->Add(*request, true, {}, {});

        UNIT_ASSERT_VALUES_EQUAL(robots->GetExpirationTime(HOST_WEB, uid), arrivalTime + ANTIROBOT_DAEMON_CONFIG.AmnestyLCookieInterval);
    }

    Y_UNIT_TEST(AddIpIdentifiedRequest) {
        const auto robots = MakeHolder<TRobotSet>();

        const TInstant arrivalTime = TInstant::Now();
        const EHostType hostType = EHostType::HOST_WEB;

        TVector<std::pair<TUid, TDuration>> uids =  {
            {TUid::FromAddr(TAddr{"0.0.0.0"}), ANTIROBOT_DAEMON_CONFIG.AmnestyIpInterval},
            {TUid::FromAddr(TAddr{"2a02:6b8:0:40c:a595:b00:885f:28c5"}), ANTIROBOT_DAEMON_CONFIG.AmnestyIpV6Interval},
        };

        for (const auto& [uid, duration] : uids) {
            const auto request = CreateRequest(uid, arrivalTime, hostType);
            robots->Add(*request, true, {}, {});

            UNIT_ASSERT_VALUES_EQUAL(robots->GetExpirationTime(hostType, uid), arrivalTime + duration);
        }
    }

    Y_UNIT_TEST(RemoveExpiredIpBans) {
        const auto robots = MakeHolder<TRobotSet>();

        TInstant arrivalTime = TInstant::Now();
        EHostType hostType = EHostType::HOST_WEB;
        auto uid = TUid::FromAddr(TAddr{"0.0.0.0"});

        auto request = CreateRequest(uid, arrivalTime, hostType);
        robots->Add(*request, true, {}, {});

        UNIT_ASSERT(robots->Contains(HOST_WEB, uid));

        robots->RemoveExpired(TInstant::Max());
        UNIT_ASSERT(!robots->Contains(HOST_WEB, uid));
    }

    Y_UNIT_TEST(RemoveExpiredLCookieBans) {
        const auto robots = MakeHolder<TRobotSet>();

        TInstant arrivalTime = TInstant::Now();
        EHostType hostType = EHostType::HOST_WEB;
        auto uid = TUid::FromAddr(TAddr{"0.0.0.0"});

        auto request = CreateRequest(uid, arrivalTime, hostType);
        robots->Add(*request, true, {}, {});

        UNIT_ASSERT(robots->Contains(HOST_WEB, uid));

        robots->RemoveExpired(TInstant::Max());
        UNIT_ASSERT(!robots->Contains(HOST_WEB, uid));
    }

    Y_UNIT_TEST(RemoveOnlyAfterFullExpiration) {
        const auto robots = MakeHolder<TRobotSet>();

        const auto uid = TUid::FromAddr(TAddr{"0.0.0.0"});
        const auto now = TInstant::Now();

        const auto oldRequest = CreateRequest(uid, now - TDuration::Days(365), HOST_WEB);
        robots->Add(*oldRequest, false, {TCbbRuleId{0}}, {});

        const auto newRequest = CreateRequest(uid, now, HOST_WEB);
        robots->Add(*newRequest, true, {}, {});

        robots->RemoveExpired();
        const auto reasons = robots->GetBanReasons(HOST_WEB, uid);
        UNIT_ASSERT(reasons.Matrixnet);
        UNIT_ASSERT(!reasons.Yql);

        robots->RemoveExpired(TInstant::Max());
        UNIT_ASSERT(!robots->Contains(HOST_WEB, uid));
    }
}


// To be run with --sanitize=thread
Y_UNIT_TEST_SUITE_IMPL(TRobotSetMultiThreading, TTestAntirobotMediumBase) {
    Y_UNIT_TEST(ThreadSafety) {
        const auto robots = MakeHolder<TRobotSet>();

        struct TEntry {
            EHostType Service{};
            TUid Uid;
            TInstant Time;
        };

        TVector<TEntry> records;

        for (auto i = 0; i < 10000; i++) {
            auto uidNamespace = static_cast<TUid::ENameSpace>(RandomNumber<size_t>(TUid::ENameSpace::Count));
            const TUid uid(uidNamespace, RandomNumber<size_t>());
            auto expire = Now() - TDuration::Seconds(10) + TDuration::MicroSeconds(RandomNumber<size_t>(10'000'000));
            auto hostType = static_cast<EHostType>(RandomNumber<size_t>(EHostType::HOST_NUMTYPES));
            records.push_back({hostType, uid, expire});
        }

        TVector<std::function<void()>> lambdas;

        for (size_t i = 0; i < 100; i++) {
            lambdas.push_back([&robots, &records]() {
                for (size_t j = 0; j < 100; j++) {
                    auto record = records[RandomNumber<size_t>(records.size())];
                    robots->AddManual(record.Service, record.Uid, record.Time, true, {}, {});
                }
            });
        }

        for (size_t i = 0; i < 100; i++) {
            lambdas.push_back([&robots, &records]() {
                for (size_t j = 0; j < 100; j++) {
                    auto record = records[RandomNumber<size_t>(records.size())];
                    robots->Contains(record.Service, record.Uid);
                }
            });
        }

        for (size_t i = 0; i < 100; i++) {
            lambdas.push_back([&robots, &records]() {
                for (size_t j = 0; j < 100; j++) {
                    auto record = records[RandomNumber<size_t>(records.size())];
                    robots->GetExpirationTime(record.Service, record.Uid);
                }
            });
        }

        for (size_t j = 0; j < 100; j++) {
            lambdas.push_back([&robots]() {
                robots->RemoveExpired();
            });
        }

        for (size_t j = 0; j < 10; j++) {
            lambdas.push_back([&robots]() {
                robots->Clear();
            });
        }

        ShuffleRange(lambdas);

        TThreadPool pool;
        pool.Start(16);

        for (const auto& lambda : lambdas) {
            pool.SafeAddFunc(lambda);
        }

        while (pool.Size() > 0) {
            Sleep(TDuration::MilliSeconds(100));
        }

        pool.Stop();
    }
}


Y_UNIT_TEST_SUITE(RobotSet) {
    const TUid UID1(TUid::IP, 1);
    const TUid UID2(TUid::IP, 2);

    Y_UNIT_TEST(IdentitySame) {
        const auto robotSet = MakeHolder<TRobotSet>();
        robotSet->Add(HOST_WEB, UID1, TInstant::MicroSeconds(1), true, {}, {});
        robotSet->Add(HOST_WEB, UID1, TInstant::MicroSeconds(10), true, {}, {});
        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 1);
    }

    Y_UNIT_TEST(IdentityDifferentUids) {
        const auto robotSet = MakeHolder<TRobotSet>();
        robotSet->Add(HOST_WEB, UID1, TInstant::MicroSeconds(1), true, {}, {});
        robotSet->Add(HOST_WEB, UID2, TInstant::MicroSeconds(10), true, {}, {});
        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 2);
    }

    Y_UNIT_TEST(IdentityDifferentServices) {
        const auto robotSet = MakeHolder<TRobotSet>();
        robotSet->Add(HOST_WEB, UID1, TInstant::MicroSeconds(1), true, {}, {});
        robotSet->Add(HOST_AVIA, UID1, TInstant::MicroSeconds(10), true, {}, {});
        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 2);
    }

    Y_UNIT_TEST(StreamSaveLoad) {
        const auto robotSet = MakeHolder<TRobotSet>();

        robotSet->Add(HOST_WEB, UID1, TInstant::MicroSeconds(1), true, {}, {});
        robotSet->Add(HOST_WEB, UID2, TInstant::MicroSeconds(10), false, {TCbbRuleId{42}}, {});
        robotSet->Add(HOST_AVIA, UID2, TInstant::MicroSeconds(1), false, {TCbbRuleId{0}}, {});
        robotSet->Add(HOST_BLOGS, UID2, TInstant::MicroSeconds(1), true, {}, {});

        TStringStream stream;
        robotSet->SerializeTo(&stream);

        TRobotSet another(&stream);

        UNIT_ASSERT(robotSet->UnsafeEquals(another));
    }

    Y_UNIT_TEST(TestAddContains) {
        const auto robotSet = MakeHolder<TRobotSet>();
        robotSet->Add(HOST_MORDA, UID1, TInstant::Now(), true, {}, {});

        UNIT_ASSERT(robotSet->Contains(HOST_MORDA, UID1));
        UNIT_ASSERT(!robotSet->Contains(HOST_MORDA, TUid(TUid::IP6, 1)));
        UNIT_ASSERT(!robotSet->Contains(HOST_MORDA, UID2));
        UNIT_ASSERT(!robotSet->Contains(HOST_YARU, UID1));
    }

    Y_UNIT_TEST(TestAddReban) {
        const auto robotSet = MakeHolder<TRobotSet>();

        const auto expires = TInstant::Now();
        robotSet->AddManual(HOST_MORDA, UID1, expires, true, {}, {});

        UNIT_ASSERT(robotSet->Contains(HOST_MORDA, UID1));
        UNIT_ASSERT_VALUES_EQUAL(robotSet->GetExpirationTime(HOST_MORDA, UID1), expires);

        const auto expiresEarly = expires - TDuration::Seconds(10);
        const auto expiresLater = expires + TDuration::Seconds(10);
        robotSet->AddManual(HOST_MORDA, UID1, expiresEarly, true, {}, {});

        UNIT_ASSERT(robotSet->Contains(HOST_MORDA, UID1));
        UNIT_ASSERT_VALUES_EQUAL(robotSet->GetExpirationTime(HOST_MORDA, UID1), expires);

        robotSet->AddManual(HOST_MORDA, UID1, expiresLater, true, {}, {});
        UNIT_ASSERT(robotSet->Contains(HOST_MORDA, UID1));
        UNIT_ASSERT_VALUES_EQUAL(robotSet->GetExpirationTime(HOST_MORDA, UID1), expiresLater);
    }

    Y_UNIT_TEST(TestGetExpirationTime) {
        const auto robotSet = MakeHolder<TRobotSet>();

        const auto expires = TInstant::Now();
        robotSet->AddManual(HOST_WEB, TUid(TUid::IP6, 1), expires, true, {}, {});

        UNIT_ASSERT_VALUES_EQUAL(robotSet->GetExpirationTime(HOST_WEB, TUid(TUid::IP6, 1)), expires);
    }

    Y_UNIT_TEST(TestRemoveExpired) {
        const auto robotSet = MakeHolder<TRobotSet>();
        auto now = TInstant::Now();

        robotSet->AddManual(HOST_YARU, UID1, now, true, {}, {});
        robotSet->AddManual(HOST_YARU, TUid(TUid::IP6, 1), now + TDuration::Seconds(1), true, {}, {});
        robotSet->AddManual(HOST_YARU, TUid(TUid::LCOOKIE, 1), now + TDuration::Seconds(2), true, {}, {});
        robotSet->AddManual(HOST_YARU, TUid(TUid::SPRAVKA, 1), now + TDuration::Seconds(3), true, {}, {});
        robotSet->AddManual(HOST_YARU, TUid(TUid::FUID, 1), now + TDuration::Seconds(3), true, {}, {});
        robotSet->AddManual(HOST_YARU, TUid(TUid::ICOOKIE, 1), now + TDuration::Seconds(3), true, {}, {});
        robotSet->AddManual(HOST_YARU, TUid(TUid::PREVIEW, 1), now + TDuration::Seconds(3), true, {}, {});
        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 7);

        robotSet->RemoveExpired(now - TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 7);

        robotSet->RemoveExpired(now);

        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 6);
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::IP6, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::LCOOKIE, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::SPRAVKA, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::FUID, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::ICOOKIE, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::PREVIEW, 1)));

        robotSet->RemoveExpired(now + TDuration::Seconds(1));

        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 5);
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::LCOOKIE, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::SPRAVKA, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::FUID, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::ICOOKIE, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::PREVIEW, 1)));

        robotSet->RemoveExpired(now + TDuration::Seconds(2));

        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 4);
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::SPRAVKA, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::FUID, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::ICOOKIE, 1)));
        UNIT_ASSERT(robotSet->Contains(HOST_YARU, TUid(TUid::PREVIEW, 1)));

        robotSet->RemoveExpired(now + TDuration::Seconds(3));
        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 0);
    }

    Y_UNIT_TEST(TestClear) {
        const auto robotSet = MakeHolder<TRobotSet>();

        robotSet->AddManual(HOST_MORDA, UID1, TInstant::MicroSeconds(1), true, {}, {});
        robotSet->AddManual(
            HOST_WEB, UID2, TInstant::Now(),
            false, {TCbbRuleId{42}}, {TCbbRuleKey(TCbbGroupId{777}, TCbbRuleId{888})}
        );

        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 2);

        robotSet->Clear();
        UNIT_ASSERT_VALUES_EQUAL(robotSet->Size(), 0);
    }

    Y_UNIT_TEST(TestAnachronisticAdd) {
        const auto robotSet = MakeHolder<TRobotSet>();

        robotSet->AddManual(HOST_MORDA, UID1, TInstant::MicroSeconds(3), false, {TCbbRuleId{1}}, {});
        robotSet->AddManual(HOST_MORDA, UID1, TInstant::MicroSeconds(1), false, {TCbbRuleId{2}}, {});
        robotSet->RemoveExpired(TInstant::MicroSeconds(2));

        TNullOutput output;
        robotSet->SerializeTo(&output);
    }

    Y_UNIT_TEST(TestCopiesUid) {
        const auto robotSet = MakeHolder<TRobotSet>();

        robotSet->AddManual(HOST_MORDA, UID1, TInstant::MicroSeconds(3), true, {}, {});

        {
            const auto uid = MakeHolder<TUid>(UID1);
            robotSet->AddManual(HOST_MORDA, *uid, TInstant::MicroSeconds(3), false, {TCbbRuleId{2}}, {});
        }

        TNullOutput output;
        robotSet->SerializeTo(&output);
    }
}


} // namespace NAntiRobot

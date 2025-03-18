#include <random>

#include <nginx/modules/strm_packager/src/common/order_manager.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>
#include <util/generic/deque.h>

using namespace NStrm::NPackager;
using namespace NStrm;

Y_UNIT_TEST_SUITE(TOrderManagerTest) {
    struct TAction {
        int Id;
        std::function<void()> Action;
    };
    Y_UNIT_TEST(TestOrder) {
        std::mt19937 randomEngine(123456);

        for (size_t dataSize : {1, 2, 3, 4, 10, 100}) {
            TVector<TMediaData> data(dataSize);

            for (size_t i = 0; i < data.size(); ++i) {
                data[i].Interval.Begin = Ti64TimeP(i);
                data[i].Interval.End = Ti64TimeP(i + 1);
            }

            TMaybe<TOrderManager> orderManager;
            TRepPromise<TMediaData> promise;

            auto test = [&orderManager, &promise, &data](const TVector<TAction>& actions) mutable {
                TVector<TMediaData> checkData;
                bool finished = false;

                promise = TRepPromise<TMediaData>::Make();
                orderManager.ConstructInPlace();
                orderManager->PutData(promise.GetFuture());

                orderManager->GetData().AddCallback([&finished, &checkData](const TRepFuture<TMediaData>::TDataRef& dataRef) mutable {
                    UNIT_ASSERT(!finished);

                    if (dataRef.Empty()) {
                        finished = true;
                        return;
                    }

                    checkData.push_back(dataRef.Data());
                });

                for (auto& a : actions) {
                    a.Action();
                }

                UNIT_ASSERT(finished);
                UNIT_ASSERT_VALUES_EQUAL(data.size(), checkData.size());
                for (size_t i = 0; i < data.size(); ++i) {
                    UNIT_ASSERT_VALUES_EQUAL(data[i].Interval.Begin, checkData[i].Interval.Begin);
                }
            };

            TVector<TAction> actions;
            int id = 0;
            actions.push_back({++id, [&orderManager]() mutable {
                                   orderManager->SetTimeBegin(Ti64TimeP(0));
                               }});

            actions.push_back({++id, [&orderManager, dataSize]() mutable {
                                   orderManager->SetTimeEnd(Ti64TimeP(dataSize));
                               }});

            for (const auto& d : data) {
                actions.push_back({++id, [&promise, d]() mutable {
                                       promise.PutData(d);
                                   }});
            }

            if (dataSize <= 4) {
                const auto comparator = [](const TAction& a, const TAction& b) {
                    return a.Id < b.Id;
                };

                std::sort(actions.begin(), actions.end(), comparator);
                do {
                    test(actions);
                } while (std::next_permutation(actions.begin(), actions.end(), comparator));
            } else {
                for (int t = 0; t < 100; ++t) {
                    std::shuffle(actions.begin(), actions.end(), randomEngine);
                    test(actions);
                }
            }
        }
    } // Y_UNIT_TEST(TestOrder)
} // Y_UNIT_TEST_SUITE(TOrderManagerTest)

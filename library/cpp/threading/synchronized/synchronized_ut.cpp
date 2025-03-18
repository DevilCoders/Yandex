#include "synchronized.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/thread/pool.h>
#include <library/cpp/deprecated/atomic/atomic.h>

namespace NThreading {
    Y_UNIT_TEST_SUITE(Synchronized) {
        struct TSyncChecker {
            TAtomic IsAccessed = false;

            bool IsBeingAccessedBySomeoneElse() {
                return !AtomicCas(&IsAccessed, true, false) || !AtomicCas(&IsAccessed, false, true);
            }
        };

        Y_UNIT_TEST(SynchronizesAccess) {
            TSynchronized<TSyncChecker> sync;

            TAtomic wasExecutedInParallel = false;
            {
                auto queue = CreateThreadPool(3);
                for (int i = 0; i < 1000; ++i) {
                    queue->SafeAddFunc([&] {
                        auto access = sync.Access();

                        // Вместо ассерта пишем в переменную, так как нельзя бросать ассерты
                        // не из главного потока
                        if (access->IsBeingAccessedBySomeoneElse()) {
                            AtomicSet(wasExecutedInParallel, true);
                        }
                    });
                }
            }
            UNIT_ASSERT(!wasExecutedInParallel);
        }

        Y_UNIT_TEST(OperatorArrowIsSynchronized) {
            TSynchronized<TSyncChecker> sync;

            TAtomic wasExecutedInParallel = false;
            {
                auto queue = CreateThreadPool(3);
                for (int i = 0; i < 1000; ++i) {
                    queue->SafeAddFunc([&] {
                        if (sync->IsBeingAccessedBySomeoneElse()) {
                            AtomicSet(wasExecutedInParallel, true);
                        }
                    });
                }
            }
            UNIT_ASSERT(!wasExecutedInParallel);
        }

        Y_UNIT_TEST(UpdatesWork) {
            TSynchronized<int> syncInt;

            const int updateCount = 1000;
            {
                auto queue = CreateThreadPool(3);
                for (int i = 0; i < updateCount; ++i) {
                    queue->SafeAddFunc([&] {
                        auto access = syncInt.Access();
                        int& theInt = *access;
                        ++theInt;
                    });
                }
            }
            UNIT_ASSERT_VALUES_EQUAL(*syncInt.Access(), updateCount);
        }

        Y_UNIT_TEST(ReturnAccessFromFunction) {
            TSynchronized<int> syncInt;

            auto GetAccess = [](TSynchronized<int>& s) {
                auto acc = s.Access();
                return acc;
            };

            const int updateCount = 1000;
            {
                auto queue = CreateThreadPool(3);
                for (int i = 0; i < updateCount; ++i) {
                    queue->SafeAddFunc([&] {
                        auto access = GetAccess(syncInt);
                        int& theInt = *access;
                        ++theInt;
                    });
                }
            }
            UNIT_ASSERT_VALUES_EQUAL(*syncInt.Access(), updateCount);
        }

        Y_UNIT_TEST(ConstType) {
            TSynchronized<const int> syncConstInt(5);
            UNIT_ASSERT_VALUES_EQUAL(*syncConstInt.Access(), 5);
            //    *syncConstInt.Access() = 12; // should not compile
        }
    }

}

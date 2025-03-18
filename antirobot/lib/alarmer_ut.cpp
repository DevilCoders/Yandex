#include <library/cpp/testing/unittest/registar.h>

#include "alarmer.h"

#include <util/string/join.h>
#include <util/system/thread.h>


namespace NAntiRobot {

Y_UNIT_TEST_SUITE(Alarmer) {
    Y_UNIT_TEST(TestSimple) {
        TAlarmer alarmer(TDuration::MilliSeconds(1));
        TVector<int> list;
        TAlarmTaskHolder taskHolder1, taskHolder2;
        taskHolder1.Reset(alarmer, TDuration::MilliSeconds(10), 2, true, [&list]() {
            list.push_back(1);
            Sleep(TDuration::MilliSeconds(10));
        });
        taskHolder2.Reset(alarmer, TDuration::MilliSeconds(10), 2, true, [&list]() {
            list.push_back(2);
            Sleep(TDuration::MilliSeconds(10));
        });
        Sleep(TDuration::MilliSeconds(100));

        UNIT_ASSERT_STRINGS_EQUAL(JoinSeq(",", list), "1,2,1,2");
    }

    Y_UNIT_TEST(TestNoDeadlock) {
        TAlarmer alarmer(TDuration::MilliSeconds(1));
        TVector<int> list;
        TAlarmTaskHolder taskHolder1, taskHolder2, innerTaskHolder;;
        THolder<TThread> thread;
        taskHolder1.Reset(alarmer, TDuration::MilliSeconds(10), 1, true, [&list, &alarmer, &innerTaskHolder, &thread]() {
            list.push_back(1);
            Sleep(TDuration::MilliSeconds(10));
            thread.Reset(new TThread([&list, &alarmer, &innerTaskHolder]() {
                list.push_back(2);
                innerTaskHolder.Reset(alarmer, TDuration::MilliSeconds(10), 1, true, [&list]() { // possible deadlock inside Reset
                    list.push_back(5);
                });
            }));
            thread->Start();

            Sleep(TDuration::MilliSeconds(10));
        });

        // второй задерживает цикл выполнения
        // в это время первый пытается внутри сделать TAlarmer::Add
        taskHolder2.Reset(alarmer, TDuration::MilliSeconds(500), 1, true, [&list]() {
            list.push_back(3);
            Sleep(TDuration::MilliSeconds(200));
            list.push_back(4);
        });

        Sleep(TDuration::MilliSeconds(1100));
        thread->Join();

        UNIT_ASSERT_STRINGS_EQUAL(JoinSeq(",", list), "1,2,3,4,5");
    }
}

} // namespace NAntiRobot


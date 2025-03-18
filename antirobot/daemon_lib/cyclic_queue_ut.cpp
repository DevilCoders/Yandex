#include <library/cpp/testing/unittest/registar.h>

#include "cyclic_queue.h"

namespace NAntiRobot {

    Y_UNIT_TEST_SUITE(TTestCyclicQueue) {

        Y_UNIT_TEST(TestSeveralCycles) {
            TCyclicQueue<int, 3> q;
            UNIT_ASSERT_EQUAL(q.GetSize(), 0);

            q.Push(10);
            UNIT_ASSERT_EQUAL(q.GetSize(), 1);
            UNIT_ASSERT_EQUAL(q.GetItem(0), 10);
            UNIT_ASSERT_EQUAL(q.GetLast(), 10);

            q.Push(20);
            UNIT_ASSERT_EQUAL(q.GetSize(), 2);
            UNIT_ASSERT_EQUAL(q.GetItem(0), 10);
            UNIT_ASSERT_EQUAL(q.GetItem(1), 20);
            UNIT_ASSERT_EQUAL(q.GetLast(), 20);

            q.Push(30);
            UNIT_ASSERT_EQUAL(q.GetSize(), 3);
            UNIT_ASSERT_EQUAL(q.GetItem(0), 10);
            UNIT_ASSERT_EQUAL(q.GetItem(1), 20);
            UNIT_ASSERT_EQUAL(q.GetItem(2), 30);
            UNIT_ASSERT_EQUAL(q.GetLast(), 30);

            q.Push(40);
            UNIT_ASSERT_EQUAL(q.GetSize(), 3);
            UNIT_ASSERT_EQUAL(q.GetItem(0), 20);
            UNIT_ASSERT_EQUAL(q.GetItem(1), 30);
            UNIT_ASSERT_EQUAL(q.GetItem(2), 40);
            UNIT_ASSERT_EQUAL(q.GetLast(), 40);

            q.Push(50);
            UNIT_ASSERT_EQUAL(q.GetSize(), 3);
            UNIT_ASSERT_EQUAL(q.GetItem(0), 30);
            UNIT_ASSERT_EQUAL(q.GetItem(1), 40);
            UNIT_ASSERT_EQUAL(q.GetItem(2), 50);
            UNIT_ASSERT_EQUAL(q.GetLast(), 50);

            q.Push(60);
            UNIT_ASSERT_EQUAL(q.GetSize(), 3);
            UNIT_ASSERT_EQUAL(q.GetItem(0), 40);
            UNIT_ASSERT_EQUAL(q.GetItem(1), 50);
            UNIT_ASSERT_EQUAL(q.GetItem(2), 60);
            UNIT_ASSERT_EQUAL(q.GetLast(), 60);
        }
    }
}


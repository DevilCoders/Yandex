#include "ext_priority_queue.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(PriorityTopN) {
    Y_UNIT_TEST(Normal) {
        TPriorityTopN<TString, int> queue(2);
        queue.push("1", 1);
        queue.push("2", 2);
        queue.push("3", 3);
        queue.push("1", 1);
        UNIT_ASSERT_STRINGS_EQUAL(queue.ytop(), "2");
        queue.pop();
        UNIT_ASSERT_STRINGS_EQUAL(queue.ytop(), "3");
        queue.pop();
        UNIT_ASSERT(queue.empty());
    }
    Y_UNIT_TEST(NormalHolderMove) {
        TPriorityTopN<THolder<TString>, int> queue(2);
        THolder<TString> d;
        d = MakeHolder<TString>("1");
        queue.push(std::move(d), 1);
        d = MakeHolder<TString>("2");
        queue.push(std::move(d), 2);
        d = MakeHolder<TString>("3");
        queue.push(std::move(d), 3);
        d = MakeHolder<TString>("1");
        queue.push(std::move(d), 1);
        UNIT_ASSERT_STRINGS_EQUAL(*queue.ytop(), "2");
        queue.pop();
        UNIT_ASSERT_STRINGS_EQUAL(*queue.ytop(), "3");
        queue.pop();
        UNIT_ASSERT(queue.empty());
    }
    Y_UNIT_TEST(Empty) {
        TPriorityTopN<TString, int> queue(0);
        queue.push("something", 1);
        UNIT_ASSERT(queue.empty());
    }
    Y_UNIT_TEST(Shrink) {
        TPriorityTopN<TString, int> queue(3);
        queue.push("1", 1);
        queue.push("2", 2);
        queue.push("3", 3);
        queue.push("1", 1);
        queue.push("1", 1);
        queue.push("2", 2);
        queue.push("2", 2);
        queue.ShrinkToSharpBorder();
        UNIT_ASSERT_STRINGS_EQUAL(queue.ytop(), "3");
        queue.pop();
        UNIT_ASSERT(queue.empty());
    }
    Y_UNIT_TEST(ShrinkStayExact) {
        TPriorityTopN<TString, int> queue(2);
        queue.push("1", 1);
        queue.push("2", 2);
        queue.push("3", 3);
        queue.push("1", 1);
        queue.ShrinkToSharpBorder();
        UNIT_ASSERT_STRINGS_EQUAL(queue.ytop(), "2");
        queue.pop();
        UNIT_ASSERT_STRINGS_EQUAL(queue.ytop(), "3");
        queue.pop();
        UNIT_ASSERT(queue.empty());
    }
}

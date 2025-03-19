#include <library/cpp/testing/unittest/registar.h>

#include "urlnorm.h"

class TUrlHashValTest: public TTestBase {
        UNIT_TEST_SUITE(TUrlHashValTest);
            UNIT_TEST(Test);
        UNIT_TEST_SUITE_END();
    public:
        void Test();
    private:
        void SingleTest(int ret_res, ui64 hash_res, const char *path, bool caseSensitive) {
            ui64 fnv = 0;
            int ret = UrlHashVal(fnv, path, caseSensitive);
            UNIT_ASSERT_VALUES_EQUAL_C(ret, ret_res, path);
            if (ret_res == 0)
                UNIT_ASSERT_VALUES_EQUAL_C(fnv, hash_res, path);
        }
        void SingleTestCaseInsensitive(int ret_res, ui64 hash_res, const char *path) {
            ui64 fnv = 0;
            int ret = UrlHashValCaseInsensitive(fnv, path);
            UNIT_ASSERT_VALUES_EQUAL_C(ret, ret_res, path);
            if (ret_res == 0)
                UNIT_ASSERT_VALUES_EQUAL_C(fnv, hash_res, path);
        }
};

UNIT_TEST_SUITE_REGISTRATION(TUrlHashValTest);

void TUrlHashValTest::Test() {
    SingleTest(0, ULL(17303248704233318424), "/hello", false);
    SingleTest(0, ULL(17303248704233318424), "/hello", true);
    SingleTest(0, ULL(17303248704233318424), "/Hello", false);
    SingleTest(0, ULL(14236859156872049912), "/Hello", true);
    SingleTest(0, ULL(14695981039346656037), "?", false);
    SingleTest(0, ULL(14695981039346656037), "?", true);
    SingleTest(0, ULL(14695981039346656037), "", false);
    SingleTest(0, ULL(14695981039346656037), "&", false);
    SingleTest(0, ULL(14695981039346656037), "&", true);

    SingleTestCaseInsensitive(0, ULL(17303248704233318424), "/hello");
    SingleTestCaseInsensitive(0, ULL(17303248704233318424), "/HeLLo");

    SingleTest(0, ULL(709616145197150583), "/scripts/db2www.exe/alex/information.d2w/HELP?GUID=49&&PublGoTo=204&&PublCurPart=193&&PublView=SHORT&&PublSort=asc&&PublItem=0&&UserName=guest&&UserPass=guest&&WhatToShow=1&&ViewMode=VIEW&&tbview=1&&D2WSection=FIND", false);

    SingleTest(0, ULL(8095477917607005527), "/scripts/db2www.exe/alex/information.d2w/HELP?GUID=49&&PublGoTo=204&&PublCurPart=193&&PublView=SHORT&&PublSort=asc&&PublItem=0&&UserName=guest&&UserPass=guest&&WhatToShow=1&&ViewMode=VIEW&&tbview=1&&D2WSection=FIND", true);
}



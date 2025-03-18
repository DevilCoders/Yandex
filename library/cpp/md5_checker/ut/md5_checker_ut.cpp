#include "md5_mapping.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/system/execpath.h>
#include <util/thread/pool.h>

Y_UNIT_TEST_SUITE(TMd5CheckerTest) {
    Y_UNIT_TEST(TestSelfMd5) {
        TThreadPool q;
        q.Start(1, 10);
        const TMD5Info& info1 = TryGetFileMD5FromMap(GetExecPath(), q);
        UNIT_ASSERT_VALUES_EQUAL(info1.MD5, "BUSY");
        UNIT_ASSERT_VALUES_EQUAL(info1.Error, "");

        const TMD5Info& info2 = GetFileMD5FromMap(GetExecPath());
        UNIT_ASSERT(info2.MD5 != "" && info2.MD5 != "BUSY");
        UNIT_ASSERT_VALUES_EQUAL(info2.Error, "");

        const TMD5Info& info3 = TryGetFileMD5FromMap(GetExecPath(), q);
        UNIT_ASSERT(info3.MD5 != "" && info3.MD5 != "BUSY");
        UNIT_ASSERT_VALUES_EQUAL(info3.Error, "");
    }
}

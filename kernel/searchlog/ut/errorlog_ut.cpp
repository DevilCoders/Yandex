#include <library/cpp/testing/unittest/registar.h>

#include <library/cpp/logger/null.h>
#include <kernel/searchlog/errorlog.h>

Y_UNIT_TEST_SUITE(TSearchLog) {
    void Write(TErrorLog& errorLog, ELogPriority priority, const TString& text) {
        (*(errorLog.CreateLogEntry(priority, __LOCATION__)).Get()) << text;
    }

    Y_UNIT_TEST(KeepLastEntry) {
        const TString test1 = "+++ test 1 +++";
        const TString test2 = "+++ test 2 +++";
        const TString test3 = "+++ test 3 +++";
        const TString test4 = "+++ test 4 +++";
        const TString testInfo = "+++ test info +++";

        TErrorLog errorLog;
        errorLog.ResetBackend(MakeHolder<TNullLogBackend>());

        Write(errorLog, TLOG_EMERG, test4);
        Write(errorLog, TLOG_EMERG, test1);
        UNIT_ASSERT(!errorLog.LastEntry(TLOG_EMERG).Contains(test4)
            && errorLog.LastEntry(TLOG_EMERG).Contains(test1));

        Write(errorLog, TLOG_CRIT, test3);
        Write(errorLog, TLOG_CRIT, test2);
        UNIT_ASSERT(!errorLog.LastEntry(TLOG_CRIT).Contains(test3)
            && errorLog.LastEntry(TLOG_CRIT).Contains(test2));

        Write(errorLog, TLOG_ERR, test4);
        Write(errorLog, TLOG_ERR, test3);
        UNIT_ASSERT(!errorLog.LastEntry(TLOG_ERR).Contains(test4)
            && errorLog.LastEntry(TLOG_ERR).Contains(test3));

        Write(errorLog, TLOG_WARNING, test3);
        Write(errorLog, TLOG_WARNING, test2);
        UNIT_ASSERT_EQUAL(errorLog.LastEntry(TLOG_WARNING), "");

        Write(errorLog, TLOG_INFO, test3);
        Write(errorLog, TLOG_INFO, test2);
        UNIT_ASSERT_EQUAL(errorLog.LastEntry(TLOG_INFO), "");

        errorLog.KeepLastEntry(TLOG_INFO, true); //change default
        Write(errorLog, TLOG_INFO, test4);
        Write(errorLog, TLOG_INFO, testInfo);
        UNIT_ASSERT(!errorLog.LastEntry(TLOG_INFO).Contains(test4)
            && errorLog.LastEntry(TLOG_INFO).Contains(testInfo));

        errorLog.KeepLastEntry(TLOG_INFO, false); //restore default
        Write(errorLog, TLOG_INFO, testInfo);
        UNIT_ASSERT_EQUAL(errorLog.LastEntry(TLOG_INFO), "");
    }

    Y_UNIT_TEST(TestRequestInfo) {
        GET_ERRORLOG.ResetBackend(MakeHolder<TNullLogBackend>());
        {
            TRequestInfoForErrorLog reqInfo{"yandsearch", "text=empty&ms=proto"};
            SEARCH_ERROR << "first message";
            TString message = GET_ERRORLOG.LastEntry(TLOG_ERR);
            UNIT_ASSERT(message.Contains("first message"));
            UNIT_ASSERT(message.Contains("(yandsearch: text=empty&ms=proto)"));
        }
        SEARCH_ERROR << "second message";
        TString message = GET_ERRORLOG.LastEntry(TLOG_ERR);
        UNIT_ASSERT(message.Contains("second message"));
        UNIT_ASSERT(!message.Contains("yandsearch"));
    }
}

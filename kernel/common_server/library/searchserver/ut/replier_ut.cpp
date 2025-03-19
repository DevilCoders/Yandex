#include <kernel/common_server/library/searchserver/replier.h>

#include <kernel/common_server/library/searchserver/simple/context/replier.h>
#include <library/cpp/testing/unittest/registar.h>
#include <search/request/data/reqdata.h>


namespace {
    class TReplyContextMock: public IRDReplyContext<IReplyContext> {
    private:
        TSearchRequestData RequestData;
    protected:
        const TBlob& DoGetBuf() const override {
            ythrow yexception() << "Not implemented";
        }

    public:
        TReplyContextMock(const TString& queryString)
            : RequestData(queryString.c_str())
        {
        }

        const TSearchRequestData& GetRequestData() const override {
            return RequestData;
        }

        TSearchRequestData& MutableRequestData() override {
            return RequestData;
        }

        TInstant GetRequestStartTime() const override {
            return TInstant::MicroSeconds(RequestData.RequestBeginTime());
        }

        bool IsHttp() const override {
            ythrow yexception() << "Not implemented";
        }

        NSearch::IOutputContext& Output() override {
            ythrow yexception() << "Not implemented";
        }

        long int GetRequestedPage() const override {
            ythrow yexception() << "Not implemented";
        }

        virtual void DoMakeSimpleReply(const TBuffer& /*buf*/, int /*code*/) override {
            FAIL_LOG("Not implemented");
        }

        virtual void AddReplyInfo(const TString& /*key*/, const TString& /*value*/, const bool /*rewrite*/) override {
            FAIL_LOG("Not implemented");
        }

        virtual const TMap<TString, TString>& GetReplyHeaders() const override {
            FAIL_LOG("Not implemented");
        }
    };

    EDeadlineCorrectionResult GetCorrectionResult(const TString& queryString, const TDuration& scatterTimeout, bool sleep) {
        TReplyContextMock mock = TReplyContextMock(queryString);
        if (sleep) {
            Sleep(TDuration::MicroSeconds(1000));
        }
        return mock.DeadlineCorrection(scatterTimeout, 0.9);
    }
}

Y_UNIT_TEST_SUITE(TestReplier) {
    Y_UNIT_TEST(TestDeadlineCorrection) {
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrOK, GetCorrectionResult("timeout=10000", TDuration::Seconds(5), false));
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrOK, GetCorrectionResult("timeout=10000", TDuration::Seconds(0), false));
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrRequestExpired, GetCorrectionResult("timeout=100", TDuration::Seconds(0), true));
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrRequestExpired, GetCorrectionResult("timeout=100", TDuration::Seconds(5), true));

        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrOK, GetCorrectionResult("out=100", TDuration::Seconds(5), true));
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrOK, GetCorrectionResult("out=100", TDuration::Seconds(5), false));
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrNoDeadline, GetCorrectionResult("out=100", TDuration::Seconds(0), false));

        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrNoDeadline, GetCorrectionResult("out=100", TDuration::Seconds(0), true));
        UNIT_ASSERT_EQUAL(EDeadlineCorrectionResult::dcrIncorrectDeadline, GetCorrectionResult("timeout=zzz", TDuration::Seconds(1), false));
    }
}

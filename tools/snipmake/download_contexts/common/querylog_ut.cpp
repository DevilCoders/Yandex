#include "querylog.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TQueryLogTest) {
    Y_UNIT_TEST(TestConstructor)
    {
        using namespace NSnippets;
        TQueryLog queryLog;

        UNIT_ASSERT(queryLog.Query.empty());
        UNIT_ASSERT(queryLog.FullRequest.empty());
        UNIT_ASSERT(queryLog.CorrectedQuery.Empty());
        UNIT_ASSERT(queryLog.UserRegion.empty());
        UNIT_ASSERT(queryLog.DomRegion.empty());
        UNIT_ASSERT(queryLog.UILanguage.empty());
        UNIT_ASSERT(queryLog.RequestId.empty());
        UNIT_ASSERT(queryLog.SnipWidth.Empty());
        UNIT_ASSERT(queryLog.ReportType.Empty());
        UNIT_ASSERT(queryLog.Docs.empty());
        UNIT_ASSERT(queryLog.MRData.Empty());
    }

    Y_UNIT_TEST(TestMandatoryFieldsSerialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog;

        queryLog.Query = "SomeQuery";
        queryLog.FullRequest = "SomeFullRequest";
        queryLog.UserRegion = "213";
        queryLog.DomRegion = "213";
        queryLog.UILanguage = "ru";
        queryLog.RequestId = "#recid";

        UNIT_ASSERT_EQUAL(queryLog.ToString(), TString("query=SomeQuery\tfull-request=SomeFullRequest\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid"));
    }

    Y_UNIT_TEST(TestAllFieldsSerialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog;

        queryLog.Query = "SomeQuery";
        queryLog.FullRequest = "SomeFullRequest";
        queryLog.CorrectedQuery = TMaybe<TString>(TString("MSP"));
        queryLog.UserRegion = "213";
        queryLog.DomRegion = "213";
        queryLog.UILanguage = "ru";
        queryLog.RequestId = "#recid";
        queryLog.SnipWidth = TMaybe<TString>(TString("500"));
        queryLog.ReportType = TMaybe<TString>(TString("RT"));
        queryLog.MRData = TMaybe<TString>(TString("#DATA#"));

        UNIT_ASSERT_EQUAL(queryLog.ToString(), TString("query=SomeQuery\tfull-request=SomeFullRequest\tmsp=MSP\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid\tsnip-width=500\treport=RT\t\t#DATA#"));
    }

    Y_UNIT_TEST(TestSingleDocSerialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog;

        queryLog.Query = "SomeQuery";
        queryLog.FullRequest = "SomeFullRequest";
        queryLog.UserRegion = "213";
        queryLog.DomRegion = "213";
        queryLog.UILanguage = "ru";
        queryLog.RequestId = "#recid";

        TQueryResultDoc doc;
        doc.Source = "src1";
        doc.Url = "http://yandex.ru";
        doc.SnippetType = "st";

        queryLog.Docs.push_back(doc);

        UNIT_ASSERT_EQUAL(queryLog.ToString(), TString("query=SomeQuery\tfull-request=SomeFullRequest\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid\tdoc=src1:http://yandex.ru\tsnippettype=st"));
    }

    Y_UNIT_TEST(TestTwoDocsSerialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog;

        queryLog.Query = "SomeQuery";
        queryLog.FullRequest = "SomeFullRequest";
        queryLog.UserRegion = "213";
        queryLog.DomRegion = "213";
        queryLog.UILanguage = "ru";
        queryLog.RequestId = "#recid";

        TQueryResultDoc doc1;
        doc1.Source = "src1";
        doc1.Url = "http://yandex.ru";
        doc1.SnippetType = "st1";
        queryLog.Docs.push_back(doc1);

        TQueryResultDoc doc2;
        doc2.Source = "src2";
        doc2.Url = "http://google.com";
        doc2.SnippetType = "st2";
        queryLog.Docs.push_back(doc2);

        UNIT_ASSERT_EQUAL(queryLog.ToString(), TString("query=SomeQuery\tfull-request=SomeFullRequest\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid\tdoc=src1:http://yandex.ru\tsnippettype=st1\tdoc=src2:http://google.com\tsnippettype=st2"));
    }

    Y_UNIT_TEST(TestMandatoryFieldsDeserialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog = TQueryLog::FromString(TString("query=SomeQuery\tfull-request=SomeFullRequest\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid"));

        UNIT_ASSERT_EQUAL(queryLog.Query, "SomeQuery");
        UNIT_ASSERT_EQUAL(queryLog.FullRequest, "SomeFullRequest");
        UNIT_ASSERT(queryLog.CorrectedQuery.Empty());
        UNIT_ASSERT_EQUAL(queryLog.UserRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.DomRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.UILanguage, "ru");
        UNIT_ASSERT_EQUAL(queryLog.RequestId, "#recid");
        UNIT_ASSERT(queryLog.SnipWidth.Empty());
        UNIT_ASSERT(queryLog.ReportType.Empty());
        UNIT_ASSERT(queryLog.Docs.empty());
        UNIT_ASSERT(queryLog.MRData.Empty());
    }

    Y_UNIT_TEST(TestAllFieldsDeserialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog = TQueryLog::FromString(TString("query=SomeQuery\tfull-request=SomeFullRequest\tmsp=MSP\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid\tsnip-width=500\treport=RT\t\t#DATA#"));

        UNIT_ASSERT_EQUAL(queryLog.Query, "SomeQuery");
        UNIT_ASSERT_EQUAL(queryLog.FullRequest, "SomeFullRequest");
        UNIT_ASSERT_EQUAL(queryLog.CorrectedQuery.GetRef(), "MSP");
        UNIT_ASSERT_EQUAL(queryLog.UserRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.DomRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.UILanguage, "ru");
        UNIT_ASSERT_EQUAL(queryLog.RequestId, "#recid");
        UNIT_ASSERT_EQUAL(queryLog.SnipWidth.GetRef(), "500");
        UNIT_ASSERT_EQUAL(queryLog.ReportType.GetRef(), "RT");
        UNIT_ASSERT(queryLog.Docs.empty());
        UNIT_ASSERT_EQUAL(queryLog.MRData.GetRef(), "#DATA#");
    }

    Y_UNIT_TEST(TestSingleDocDeserialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog = TQueryLog::FromString(TString("query=SomeQuery\tfull-request=SomeFullRequest\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid\tdoc=src1:http://yandex.ru\tsnippettype=st"));

        UNIT_ASSERT_EQUAL(queryLog.Query, "SomeQuery");
        UNIT_ASSERT_EQUAL(queryLog.FullRequest, "SomeFullRequest");
        UNIT_ASSERT(queryLog.CorrectedQuery.Empty());
        UNIT_ASSERT_EQUAL(queryLog.UserRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.DomRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.UILanguage, "ru");
        UNIT_ASSERT_EQUAL(queryLog.RequestId, "#recid");
        UNIT_ASSERT(queryLog.SnipWidth.Empty());
        UNIT_ASSERT(queryLog.ReportType.Empty());

        UNIT_ASSERT_EQUAL(queryLog.Docs.size(), 1);

        UNIT_ASSERT_EQUAL(queryLog.Docs[0].Source, "src1");
        UNIT_ASSERT_EQUAL(queryLog.Docs[0].Url, "http://yandex.ru");
        UNIT_ASSERT_EQUAL(queryLog.Docs[0].SnippetType, "st");

        UNIT_ASSERT(queryLog.MRData.Empty());
    }

    Y_UNIT_TEST(TestTwoDocsDeserialization)
    {
        using namespace NSnippets;
        TQueryLog queryLog = TQueryLog::FromString(TString("query=SomeQuery\tfull-request=SomeFullRequest\tuser-region=213\tdom-region=213\tuil=ru\treqid=#recid\tdoc=src1:http://yandex.ru\tsnippettype=st1\tdoc=src2:http://google.com\tsnippettype=st2"));

        UNIT_ASSERT_EQUAL(queryLog.Query, "SomeQuery");
        UNIT_ASSERT_EQUAL(queryLog.FullRequest, "SomeFullRequest");
        UNIT_ASSERT(queryLog.CorrectedQuery.Empty());
        UNIT_ASSERT_EQUAL(queryLog.UserRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.DomRegion, "213");
        UNIT_ASSERT_EQUAL(queryLog.UILanguage, "ru");
        UNIT_ASSERT_EQUAL(queryLog.RequestId, "#recid");
        UNIT_ASSERT(queryLog.SnipWidth.Empty());
        UNIT_ASSERT(queryLog.ReportType.Empty());

        UNIT_ASSERT_EQUAL(queryLog.Docs.size(), 2);

        UNIT_ASSERT_EQUAL(queryLog.Docs[0].Source, "src1");
        UNIT_ASSERT_EQUAL(queryLog.Docs[0].Url, "http://yandex.ru");
        UNIT_ASSERT_EQUAL(queryLog.Docs[0].SnippetType, "st1");

        UNIT_ASSERT_EQUAL(queryLog.Docs[1].Source, "src2");
        UNIT_ASSERT_EQUAL(queryLog.Docs[1].Url, "http://google.com");
        UNIT_ASSERT_EQUAL(queryLog.Docs[1].SnippetType, "st2");

        UNIT_ASSERT(queryLog.MRData.Empty());
    }

    Y_UNIT_TEST(TestFull)
    {
        using namespace NSnippets;

        TQueryLog queryLog1;
        queryLog1.Query = "SomeQuery";
        queryLog1.FullRequest = "SomeFullRequest";
        queryLog1.CorrectedQuery = TMaybe<TString>(TString("MSP"));
        queryLog1.UserRegion = "213";
        queryLog1.DomRegion = "213";
        queryLog1.UILanguage = "ru";
        queryLog1.RequestId = "#recid";
        queryLog1.SnipWidth = TMaybe<TString>(TString("500"));
        queryLog1.ReportType = TMaybe<TString>(TString("RT"));

        TQueryResultDoc doc1;
        doc1.Source = "src1";
        doc1.Url = "http://yandex.ru";
        doc1.SnippetType = "st1";
        queryLog1.Docs.push_back(doc1);

        TQueryResultDoc doc2;
        doc2.Source = "src2";
        doc2.Url = "http://google.com";
        doc2.SnippetType = "st2";
        queryLog1.Docs.push_back(doc2);

        queryLog1.MRData = TMaybe<TString>(TString("#DATA#"));

        TQueryLog queryLog2 = TQueryLog::FromString(queryLog1.ToString());

        UNIT_ASSERT_EQUAL(queryLog1.Query, queryLog2.Query);
        UNIT_ASSERT_EQUAL(queryLog1.FullRequest, queryLog2.FullRequest);
        UNIT_ASSERT_EQUAL(queryLog1.CorrectedQuery.Empty(), queryLog2.CorrectedQuery.Empty());
        UNIT_ASSERT_EQUAL(*queryLog1.CorrectedQuery.Get(), *queryLog2.CorrectedQuery.Get());
        UNIT_ASSERT_EQUAL(queryLog1.UserRegion, queryLog2.UserRegion);
        UNIT_ASSERT_EQUAL(queryLog1.DomRegion, queryLog2.DomRegion);
        UNIT_ASSERT_EQUAL(queryLog1.UILanguage, queryLog2.UILanguage);
        UNIT_ASSERT_EQUAL(queryLog1.RequestId, queryLog2.RequestId);
        UNIT_ASSERT_EQUAL(queryLog1.SnipWidth.Empty(), queryLog2.SnipWidth.Empty());
        UNIT_ASSERT_EQUAL(*queryLog1.SnipWidth.Get(), *queryLog2.SnipWidth.Get());
        UNIT_ASSERT_EQUAL(queryLog1.ReportType.Empty(), queryLog2.ReportType.Empty());
        UNIT_ASSERT_EQUAL(*queryLog1.ReportType.Get(), *queryLog2.ReportType.Get());

        UNIT_ASSERT_EQUAL(queryLog1.Docs.size(), queryLog2.Docs.size());
        for (size_t i = 0; i < queryLog1.Docs.size(); i++) {
            UNIT_ASSERT_EQUAL(queryLog1.Docs[i].Source, queryLog2.Docs[i].Source);
            UNIT_ASSERT_EQUAL(queryLog1.Docs[i].Url, queryLog2.Docs[i].Url);
            UNIT_ASSERT_EQUAL(queryLog1.Docs[i].SnippetType, queryLog2.Docs[i].SnippetType);
        }

        UNIT_ASSERT_EQUAL(queryLog1.MRData.Empty(), queryLog2.MRData.Empty());
        UNIT_ASSERT_EQUAL(*queryLog1.MRData.Get(), *queryLog2.MRData.Get());
    }

}

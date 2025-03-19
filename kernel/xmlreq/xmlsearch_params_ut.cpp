#include <library/cpp/testing/unittest/registar.h>
#include "xmlsearch_params.h"

namespace NAntiRobot {

    const TString xml1 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<request>\
    <query>query with spaces</query>\
    <sortby>tm</sortby>\
    <page>2</page>\
</request>";

    const TString xml2 =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\
<request>\
    <query> query </query>\
</request>";

    const TString xml3 =
"<?xml version='1.0' encoding='UTF-8'?>\
<request>\
    <query>url=yandex.ru &amp;&amp; текстовый запрос  </query>\
        <page>0</page>\
        <max-title-length>75</max-title-length>\
        <max-headline-length>300</max-headline-length>\
        <max-passage-length>400</max-passage-length>\
        <max-text-length>500</max-text-length>\
        <sortby priority='yes' order='descending'>rlv</sortby>\
        <groupings>\
            <groupby attr='ih' mode='deep' groups-on-page='10' docs-in-group='1' curcateg='2'/>\
            <groupby attr='d' mode='deep' groups-on-page='15' docs-in-group='2' depth='1' killdup='yes'/>\
        </groupings>\
        <reqid>request-id</reqid>\
        <nocache/>\
</request>";

    const TString xml4 =
"<?xml version='1.0' encoding='utf-8'?>\
<request>\
<query><![CDATA[тест /&& пряник]]></query>\
<max-title-length>70</max-title-length>\
<page>0</page>\
</request>";

    bool operator==(const TXmlSearchRequest::TGroupByList& r1, const TXmlSearchRequest::TGroupByList& r2) {
        if (r1.size() == r2.size()) {
            bool res = true;
            for (size_t i = 0; i < r1.size(); i++) {
                res = res
                    && r1[i].Attr == r2[i].Attr
                    && r1[i].Mode == r2[i].Mode
                    && r1[i].GroupsOnPage == r2[i].GroupsOnPage
                    && r1[i].DocsInGroup == r2[i].DocsInGroup
                    && r1[i].CurCateg == r2[i].CurCateg
                    && r1[i].Depth == r2[i].Depth
                    && r1[i].KillDup == r2[i].KillDup
                    ;
                if (!res)
                    return false;
            }
            return res;
        } else
            return false;
    }

    bool operator==(const TXmlSearchRequest& r1, const TXmlSearchRequest& r2) {
        return r1.Query == r2.Query
            && r1.SortBy == r2.SortBy
            && r1.Page == r2.Page
            && r1.MaxPassages == r2.MaxPassages
            && r1.MaxPassageLength == r2.MaxPassageLength
            && r1.MaxTitleLength == r2.MaxTitleLength
            && r1.MaxHeadlineLength == r2.MaxHeadlineLength
            && r1.MaxTextLength == r2.MaxTextLength
            && r1.Groupings == r2.Groupings
            && r1.ReqId == r2.ReqId
            && r1.NoCache == r2.NoCache
            ;
    }

    class TTestXmlSearchParse : public TTestBase {
    public:

        UNIT_TEST_SUITE(TTestXmlSearchParse);
            UNIT_TEST(TestXmlSearchParse);
            UNIT_TEST(TestCgiMake);
        UNIT_TEST_SUITE_END();

        void TestXmlSearchParse()
        {
            {
                TXmlSearchRequest request;
                TStringInput input(xml1);
                UNIT_ASSERT_EQUAL(ParseXmlSearch(input, request), true);
            }

            {
                TXmlSearchRequest canon;
                canon.Query = "query with spaces";
                canon.SortBy = "tm";
                canon.Page = "2";

                TXmlSearchRequest request;
                TStringInput input(xml1);
                ParseXmlSearch(input, request);

                UNIT_ASSERT_EQUAL(request, canon);
            }
            {
                TXmlSearchRequest request;
                TStringInput input(xml2);

                TXmlSearchRequest canon;
                canon.Query = " query ";
                ParseXmlSearch(input, request);
                UNIT_ASSERT_EQUAL(request, canon);
            }

            {
                TXmlSearchRequest request;
                TStringInput input(xml3);

                TXmlSearchRequest canon;
                canon.Query = "url=yandex.ru && текстовый запрос  ";
                canon.Page = "0";
                canon.SortBy = "rlv";

                canon.MaxTitleLength = "75";
                canon.MaxHeadlineLength = "300";
                canon.MaxPassageLength = "400";
                canon.MaxTextLength = "500";
                canon.ReqId = "request-id";
                canon.NoCache = true;

                {
                    TXmlSearchRequest::TGroupBy groupBy;
                    groupBy.Attr = "ih";
                    groupBy.Mode = "deep";
                    groupBy.GroupsOnPage = "10";
                    groupBy.DocsInGroup = "1";
                    groupBy.CurCateg = "2";

                    canon.Groupings.push_back(groupBy);
                }

                {
                    TXmlSearchRequest::TGroupBy groupBy;
                    groupBy.Attr = "d";
                    groupBy.Mode = "deep";
                    groupBy.GroupsOnPage = "15";
                    groupBy.DocsInGroup = "2";
                    groupBy.Depth = "1";
                    groupBy.KillDup = "yes";

                    canon.Groupings.push_back(groupBy);
                }

                ParseXmlSearch(input, request);
                UNIT_ASSERT_EQUAL(request, canon);
            }
        }

        void TestCgiMake() {
            {
                TXmlSearchRequest request;
                TStringInput input(xml3);

                ParseXmlSearch(input, request);

                TString cgiResult = request.ToCgiString();
                UNIT_ASSERT_EQUAL(cgiResult,
                    "query=\
url%3Dyandex.ru+%26%26+%D1%82%D0%B5%D0%BA%D1%81%D1%82%D0%BE%D0%B2%D1%8B%D0%B9+%D0%B7%D0%B0%D0%BF%D1%80%D0%BE%D1%81++\
&page=0\
&sortby=rlv\
&max-passage-length=400\
&max-title-length=75\
&max-headline-length=300\
&max-text-length=500\
&reqid=request-id\
&nocache=\
&groupby=attr%3Dih.mode%3Ddeep.groups-on-page%3D10.docs-in-group%3D1.curcateg%3D2.\
&groupby=attr%3Dd.mode%3Ddeep.groups-on-page%3D15.docs-in-group%3D2.depth%3D1.killdup%3Dyes."
                    );
            }


            {
                TXmlSearchRequest request;
                TStringInput input(xml4);

                ParseXmlSearch(input, request);
                TString cgi = request.ToCgiString();

                UNIT_ASSERT_EQUAL(request.ToCgiString(),
                        "query=\
%D1%82%D0%B5%D1%81%D1%82+/%26%26+%D0%BF%D1%80%D1%8F%D0%BD%D0%B8%D0%BA\
&page=0\
&max-title-length=70");
            }
        }

    };
}

UNIT_TEST_SUITE_REGISTRATION(NAntiRobot::TTestXmlSearchParse);

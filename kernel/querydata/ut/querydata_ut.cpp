#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <kernel/querydata/client/qd_merge.h>
#include <kernel/querydata/client/qd_document_responses.h>
#include <kernel/querydata/client/querydata.h>
#include <kernel/querydata/common/qd_source_names.h>
#include <kernel/urlid/url2docid.h>

#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/singleton.h>
#include <library/cpp/string_utils/url/url.h>

class TQueryDataTest : public TTestBase {
    UNIT_TEST_SUITE(TQueryDataTest)
        UNIT_TEST(TestSimpleMergeByVersion)
        UNIT_TEST(TestSimpleMergeByPriority)
        UNIT_TEST(TestUrlResponse)
        UNIT_TEST(TestSourceNames)
    UNIT_TEST_SUITE_END();

    static const NQueryData::TSourceFactors& FindSF(const NQueryData::TQueryData& qd, TStringBuf src, TStringBuf key) {
        using namespace NQueryData;
        for (ui32 i = 0; i < qd.SourceFactorsSize(); ++i) {
            const TSourceFactors& sf = qd.GetSourceFactors(i);

            if (sf.GetSourceName() != src || sf.GetSourceKey() != key)
                continue;

            return sf;
        }

        UNIT_FAIL(TString{src} + "/" + TString{key} + " :: " + NJson::CompactifyJson(NSc::TValue::From(qd).ToJson(true), true));
        return Default<TSourceFactors>();
    }

    void Assert(const NQueryData::TQueryData& qd, TStringBuf src, TStringBuf key, TStringBuf fac, TStringBuf val, ui64 ver, bool rt = false) {
        using namespace NQueryData;

        bool found = false;
        const TSourceFactors& sf = FindSF(qd, src, key);
        for (ui32 j = 0; j < sf.FactorsSize(); ++j) {
            const TFactor& f = sf.GetFactors(j);

            if (f.GetName() != fac)
                continue;

            UNIT_ASSERT_C(!found, qd.Utf8DebugString());
            found = true;
            UNIT_ASSERT_VALUES_EQUAL_C(f.GetStringValue(), val, qd.Utf8DebugString());
            UNIT_ASSERT_VALUES_EQUAL_C(sf.GetVersion(), ver, qd.Utf8DebugString());
            UNIT_ASSERT_VALUES_EQUAL_C(sf.GetRealTime(), rt, qd.Utf8DebugString());
        }

        UNIT_ASSERT_C(found, qd.Utf8DebugString());
    }

    void DoBuildQueryData(NQueryData::TQueryData& res, TStringBuf json) {
        NQueryData::NTests::BuildQDSimpleJson(res, json);
    }

    struct TFLess {
        bool operator()(const NQueryData::TFactor& a, const NQueryData::TFactor& b) {
            return a.GetName() < b.GetName();
        }
    };

    void SortFactors(NQueryData::TQueryData& qd, TString& jsons) {
        using namespace NQueryData;
        jsons.append('[');
        for (ui32 i = 0, sz = qd.SourceFactorsSize(); i < sz; ++i) {
            TSourceFactors* sf = qd.MutableSourceFactors(i);
            ::google::protobuf::RepeatedPtrField<TFactor>* ff = sf->MutableFactors();
            Sort(ff->begin(), ff->end(), TFLess());
            jsons.append(sf->HasJson() ? sf->GetJson() : "null").append(',');
            if (sf->HasJson())
                sf->SetJson("MOVED");
        }
        jsons.append(']');
    }

    void AssertQD(const NQueryData::TQueryData& qd, TStringBuf ref, TStringBuf json) {
        using namespace NQueryData;
        TString qdjson;
        TQueryData qdcpy;
        qdcpy.CopyFrom(qd);
        SortFactors(qdcpy, qdjson);
        NSc::TValue qdv = NSc::TValue::From(qdcpy);
        NSc::TValue refv = NSc::TValue::FromJson(ref);
        UNIT_ASSERT_JSON_EQ_JSON(qdv, refv);
        NSc::TValue refj = NSc::TValue::FromJson(json);
        NSc::TValue qdj = NSc::TValue::FromJson(qdjson);
        UNIT_ASSERT_JSON_EQ_JSON(qdj, refj);
    }

    void TestSimpleMergeByVersion() {
        using namespace NQueryData;
        TQueryData result;
        TVector<TQueryData> input(3);
        DoBuildQueryData(result, "{src:{key:{a:b,m:n,JSON:{a:b,m:n},VER:9}}}");
        DoBuildQueryData(input[0], "{src:{key:{a:1,p:q,JSON:{a:c,p:q},VER:11}}}");
        DoBuildQueryData(input[1], "{src:{key:{a:d,r:s,JSON:{a:d,r:s},VER:10}}}");
        DoBuildQueryData(input[2], "{src:{key:{a:e,t:u,VER:11}}}");

        {
            MergeQueryDataSimple(result, input);
            AssertQD(result, "{SourceFactors:[{SourceKeyType:KT_QUERY_EXACT,Json:MOVED,Version:11,SourceKey:key,Factors:["
                            "{FloatValue:1,Name:a},"
                            "{StringValue:q,Name:p}"
                            "],SourceName:src}]}", "[{a:c,p:q}]");

            MergeQueryDataSimple(result, input);
            AssertQD(result, "{SourceFactors:[{SourceKeyType:KT_QUERY_EXACT,Json:MOVED,Version:11,SourceKey:key,Factors:["
                            "{FloatValue:1,Name:a},"
                            "{StringValue:q,Name:p}"
                            "],SourceName:src}]}", "[{a:c,p:q}]");
        }
    }

    void DoTestSimpleMergeByPriority(NQueryData::TQueryData& result, TVector<NQueryData::TQueryData>& input,
                                     TStringBuf qd, TStringBuf expectedQD, TStringBuf expectedJson) {
        using namespace NQueryData;
        input.emplace_back();
        NTests::BuildQDJson(input.back(), qd);

        MergeQueryDataSimple(result, input);
        AssertQD(result, expectedQD, expectedJson);

        MergeQueryDataSimple(result, input);
        AssertQD(result, expectedQD, expectedJson);
    }

    void TestSimpleMergeByPriority() {
        using namespace NQueryData;
        TVector<TQueryData> input;
        TQueryData result;

        DoTestSimpleMergeByPriority(result, input
                                    , "{SourceFactors:[{"
                            "SourceKeyType:KT_QUERY_EXACT,"
                            "Version:10,"
                            "SourceKey:key,"
                            "SourceName:src,"
                            "Factors:[{StringValue:b,Name:a}],"
                            "SourceSubkeys:[{Key:rrr,Type:region},{Key:fff,Type:tld},{Key:xxx,Type:yid}],"
                            "Json:{x:y},"
                            "MergeTraits:{Priority:1}"
                            "}]}"
                                    , "{SourceFactors:["
                            "{SourceKey:key"
                            ",SourceSubkeys:["
                                "{Key:rrr,Type:KT_USER_REGION},"
                                "{Key:fff,Type:KT_SERP_TLD},"
                                "{Key:xxx,Type:KT_USER_ID}"
                            "],Json:MOVED,SourceName:src,MergeTraits:{Priority:1}"
                            ",Factors:[{StringValue:b,Name:a}]"
                            ",SourceKeyType:KT_QUERY_EXACT,Version:10}"
                            "]}"
                                    , "[{x:y}]");

        DoTestSimpleMergeByPriority(result, input
                                    , "{SourceFactors:[{"
                            "SourceKeyType:KT_QUERY_EXACT,"
                            "Version:9,"
                            "SourceKey:key,"
                            "SourceName:src,"
                            "Factors:[{StringValue:n,Name:m}],"
                            "SourceSubkeys:[{Key:rrr,Type:region},{Key:fff,Type:tld},{Key:yyy,Type:yid}],"
                            "Json:{p:q},"
                            "MergeTraits:{Priority:2}"
                            "}]}"
                                    , "{SourceFactors:["
                             "{SourceKey:key"
                             ",SourceSubkeys:["
                                 "{Key:rrr,Type:KT_USER_REGION},"
                                 "{Key:fff,Type:KT_SERP_TLD},"
                                 "{Key:xxx,Type:KT_USER_ID}"
                             "],Json:MOVED,SourceName:src,MergeTraits:{Priority:1}"
                             ",Factors:[{StringValue:b,Name:a}]"
                             ",SourceKeyType:KT_QUERY_EXACT,Version:10}"
                             ",{SourceKey:key"
                             ",SourceSubkeys:["
                                 "{Key:rrr,Type:KT_USER_REGION},"
                                 "{Key:fff,Type:KT_SERP_TLD},"
                                 "{Key:yyy,Type:KT_USER_ID}"
                             "],Json:MOVED,SourceName:src,MergeTraits:{Priority:2}"
                             ",Factors:[{StringValue:n,Name:m}]"
                             ",SourceKeyType:KT_QUERY_EXACT,Version:9}"
                            "]}"
                                    , "[{x:y},{p:q}]");

        DoTestSimpleMergeByPriority(result, input
                                    , "{SourceFactors:[{"
                            "SourceKeyType:KT_QUERY_EXACT,"
                            "Version:9,"
                            "SourceKey:key,"
                            "SourceName:src,"
                            "Factors:[{StringValue:q,Name:p}],"
                            "SourceSubkeys:[{Key:mmm,Type:region},{Key:nnn,Type:tld},{Key:xxx,Type:yid}],"
                            "Json:{r:s},"
                            "MergeTraits:{Priority:2}"
                            "}]}"
                                    , "{SourceFactors:["
                             "{SourceKey:key"
                             ",SourceSubkeys:["
                                 "{Key:mmm,Type:KT_USER_REGION},"
                                 "{Key:nnn,Type:KT_SERP_TLD},"
                                 "{Key:xxx,Type:KT_USER_ID}"
                             "],Json:MOVED,SourceName:src,MergeTraits:{Priority:2}"
                             ",Factors:[{StringValue:q,Name:p}]"
                             ",SourceKeyType:KT_QUERY_EXACT,Version:9}"
                             ",{SourceKey:key"
                             ",SourceSubkeys:["
                                 "{Key:rrr,Type:KT_USER_REGION},"
                                 "{Key:fff,Type:KT_SERP_TLD},"
                                 "{Key:yyy,Type:KT_USER_ID}"
                             "],Json:MOVED,SourceName:src,MergeTraits:{Priority:2}"
                             ",Factors:[{StringValue:n,Name:m}]"
                             ",SourceKeyType:KT_QUERY_EXACT,Version:9}"
                            "]}"
                                    , "[{r:s},{p:q}]");
    }

    struct TTestProc : public NQueryData::IDocumentResponseProcessor, public NQueryData::TDocView {
        const NQueryData::TSourceFactors* Res = nullptr;

        void OnUrl(TStringBuf url, const NQueryData::TSourceFactors& sf) override {
            Url = url;
            Res = &sf;
        }
        void OnDocId(TStringBuf docId, const NQueryData::TSourceFactors& sf) override {
            DocId = docId;
            Res = &sf;
        }
        void OnOwner(TStringBuf owner, const NQueryData::TSourceFactors& sf) override {
            Owner = owner;
            Res = &sf;
        }
    };

    struct TUrlTestCase : public NQueryData::TDocView {
        TStringBuf SourceFactors;

        bool Matches = true;

        TUrlTestCase(TStringBuf sf, const NQueryData::TDocView& doc, bool matches = true)
            : NQueryData::TDocView(doc)
            , SourceFactors(sf)
            , Matches(matches)
        {
        }

        void TestMatch(bool needSort) const {
            auto sourceFactors = NQueryData::NTests::BuildSFJson(SourceFactors);
            UNIT_ASSERT_VALUES_EQUAL_C(NQueryData::ResponseMatchesDocument(sourceFactors, *this), Matches,
                                       TStringBuilder() << SourceFactors << " " << Url << " " << Owner << " " << DocId);

            NQueryData::TQueryData qd;
            qd.AddSourceFactors()->CopyFrom(sourceFactors);

            TTestProc testProc;

            NQueryData::ProcessDocumentResponses(testProc, qd, needSort);

            UNIT_ASSERT(testProc.Res != nullptr);

            UNIT_ASSERT(testProc.Url || testProc.DocId || testProc.Owner);

            if (testProc.Url) {
                UNIT_ASSERT(testProc.Url == "url" || testProc.Url == "https://url");
            }

            if (testProc.DocId) {
                UNIT_ASSERT_VALUES_EQUAL(testProc.DocId, "ZDOCID");
            }

            if (testProc.Owner) {
                UNIT_ASSERT_VALUES_EQUAL(testProc.Owner, "categ");
            }

            const TString docId = Url2DocIdSimple("url");
            const TString httpsDocId = Url2DocIdSimple("https://url");

            ui32 docIdCnt = 0;
            ui32 ownerCnt = 0;
            ui32 urlCnt = 0;
            NQueryData::ProcessDocIdResponses([&](TStringBuf sk, const NQueryData::TSourceFactors&) {
                UNIT_ASSERT(sk == "ZDOCID" || sk == docId || sk == httpsDocId);
                docIdCnt += 1;
            }, qd, needSort);

            NQueryData::ProcessOwnerResponses([&](TStringBuf sk, const NQueryData::TSourceFactors&) {
                UNIT_ASSERT_VALUES_EQUAL(sk, "categ");
                ownerCnt += 1;
            }, qd, needSort);

            NQueryData::ProcessUrlResponses([&](TStringBuf sk, const NQueryData::TSourceFactors&) {
                UNIT_ASSERT(sk == "url" || sk == "https://url");
                urlCnt += 1;
            }, qd, needSort);

            UNIT_ASSERT_VALUES_EQUAL(docIdCnt, !!testProc.Url + !!testProc.DocId);
            UNIT_ASSERT_VALUES_EQUAL(urlCnt, !!testProc.Url);
            UNIT_ASSERT_VALUES_EQUAL(ownerCnt, !!testProc.Owner);
        }
    };


    void TestUrlResponse() {
        NQueryData::TDocView match;
        match.SetDocId("ZDOCID").SetUrl("url").SetOwner("categ");

        NQueryData::TDocView matchHttp = match;
        matchHttp.SetUrl("http://url");

        NQueryData::TDocView misMatch;
        misMatch.SetDocId("ZDOCID2").SetUrl("url2").SetOwner("categ2");

        NQueryData::TDocView matchHttps = match;
        matchHttps.SetUrl("https://url");

        for (const auto& sf : std::initializer_list<TUrlTestCase>{
            TUrlTestCase("{SourceKeyType:KT_DOCID,SourceKey:ZDOCID}", match),
            TUrlTestCase("{SourceKeyType:KT_DOCID,SourceKey:ZDOCID}", misMatch, false),
            TUrlTestCase("{SourceKeyType:KT_SNIPDOCID,SourceKey:ZDOCID}", match),
            TUrlTestCase("{SourceKeyType:KT_SNIPDOCID,SourceKey:ZDOCID}", misMatch, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_DOCID,Key:ZDOCID}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_DOCID,Key:ZDOCID}]}", misMatch, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_SNIPDOCID,Key:ZDOCID}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_SNIPDOCID,Key:ZDOCID}]}", misMatch, false),
            TUrlTestCase("{SourceKeyType:KT_CATEG,SourceKey:categ}", match),
            TUrlTestCase("{SourceKeyType:KT_CATEG,SourceKey:categ}", misMatch, false),
            TUrlTestCase("{SourceKeyType:KT_SNIPCATEG,SourceKey:categ}", match),
            TUrlTestCase("{SourceKeyType:KT_SNIPCATEG,SourceKey:categ}", misMatch, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG,Key:categ}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG,Key:categ}]}", misMatch, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_SNIPCATEG,Key:categ}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_SNIPCATEG,Key:categ}]}", misMatch, false),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:url}", match),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:url}", matchHttp),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:'http://url'}", match),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:'https://url'}", matchHttps),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:url}", misMatch, false),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:url}", matchHttps, false),
            TUrlTestCase("{SourceKeyType:KT_URL,SourceKey:'https://url'}", match, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:url}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:url}]}", matchHttp),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:'http://url'}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:'https://url'}]}", matchHttps),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:url}]}", misMatch, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:url}]}", matchHttps, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_URL,Key:'https://url'}]}", match, false),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:url}", match),
            TUrlTestCase("{SourceKeyType:KT_SNIPCATEG_URL,SourceKey:url}", match),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c url'}", match),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c url'}", matchHttp),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c http://url'}", match),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c https://url'}", matchHttps),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c url'}", misMatch, false),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c url'}", matchHttps, false),
            TUrlTestCase("{SourceKeyType:KT_CATEG_URL,SourceKey:'c https://url'}", match, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:url}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_SNIPCATEG_URL,Key:url}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c url'}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c url'}]}", matchHttp),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c http://url'}]}", match),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c https://url'}]}", matchHttps),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c url'}]}", misMatch, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c url'}]}", matchHttps, false),
            TUrlTestCase("{SourceSubkeys:[{Type:KT_CATEG_URL,Key:'c https://url'}]}", match, false),
        }) {
            sf.TestMatch(true);
            sf.TestMatch(false);
        }
    }


    void DoTestSourceNames(TStringBuf srcName, TStringBuf expectedMain, TStringBuf expectedAux, TStringBuf expectedSrcName) {
        TStringBuf mainSrc, auxSrc;
        mainSrc = NQueryData::GetMainSourceName(srcName, auxSrc);
        UNIT_ASSERT_VALUES_EQUAL(mainSrc, expectedMain);
        UNIT_ASSERT_VALUES_EQUAL(auxSrc, expectedAux);
        UNIT_ASSERT_VALUES_EQUAL(NQueryData::ComposeSourceName(mainSrc, auxSrc), expectedSrcName);
    }

    void TestSourceNames() {
        DoTestSourceNames("x", "x", "", "x");
        DoTestSourceNames("x/y", "x", "y", "y:x");
        DoTestSourceNames("x/y", "x", "y", "y:x");
        DoTestSourceNames("y:x", "x", "y", "y:x");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQueryDataTest)

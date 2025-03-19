#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <kernel/querydata/cgi/qd_cgi_strings.h>
#include <kernel/querydata/cgi/qd_cgi.h>
#include <kernel/querydata/cgi/qd_cgi_utils.h>
#include <kernel/querydata/cgi/qd_request.h>

#include <library/cpp/testing/unittest/registar.h>

class TQueryDataCgiTest : public TTestBase {
    UNIT_TEST_SUITE(TQueryDataCgiTest)
        UNIT_TEST(TestVectorSplit)
        UNIT_TEST(TestParamSplitter)
        UNIT_TEST(TestCompressCgi)
        UNIT_TEST(TestParseLists)
        UNIT_TEST(TestCGI)
        UNIT_TEST(TestJsonArrayQdUrls)
    UNIT_TEST_SUITE_END();

    static NQueryData::TStringBufs ArrayToVector(const NSc::TArray& arr) {
        NQueryData::TStringBufs res;

        for (const auto& item : arr) {
            res.push_back(item.GetString());
        }

        return res;
    }

    void TestVectorSplit() {
        using namespace NQueryData;
        TString data;
        for (ui32 i = 0; i < 26; ++i) {
            data.append('a' + i).append(',');
        }

        TStringBufs vec;
        for (ui32 i = 0; i < 26; ++i) {
            vec.push_back(TStringBuf(data.data() + 2 * i, data.data() + 2 * i + 1));
        }

        for (ui32 sz = 0; sz <= vec.size(); ++sz) {
            TStringBufs subvec(vec.begin(), vec.begin() + sz);
            for (ui32 parts = 0; parts <= 30; ++parts) {
                TString allres;

                for (ui32 part = 0; part < 30; ++part) {
                    TString res = VectorToStringSplit(subvec, ',', part, parts);

                    if (allres && res) {
                        allres.append(',');
                    }

                    allres.append(res);
                }

                if (parts && sz) {
                    UNIT_ASSERT_VALUES_EQUAL_C(allres, data.substr(0, 2 * sz - 1), Sprintf("%u %u", sz, parts));
                } else {
                    UNIT_ASSERT_VALUES_EQUAL_C(allres, TString(), Sprintf("%u %u", sz, parts));
                }
            }
        }

        UNIT_ASSERT_VALUES_EQUAL("a,b,c,d,e,f", VectorToStringSplit(vec, ',', 0, 4));
        UNIT_ASSERT_VALUES_EQUAL("g,h,i,j,k,l", VectorToStringSplit(vec, ',', 1, 4));
        UNIT_ASSERT_VALUES_EQUAL("m,n,o,p,q,r,s", VectorToStringSplit(vec, ',', 2, 4));
        UNIT_ASSERT_VALUES_EQUAL("t,u,v,w,x,y,z", VectorToStringSplit(vec, ',', 3, 4));
    }

    void DoTestParamSplitter(ui32 parts, const NQueryData::TRequestSplitMeasure& req, const NQueryData::TRequestSplitLimits& lim) {
        UNIT_ASSERT_VALUES_EQUAL_C(parts, NQueryData::CalculatePartsCount(req, lim)
                                 , Sprintf("m(%u %u %u), l(%u %u %u)",
                                           req.SharedSize, req.SplittableCount, req.SplittableSize,
                                           lim.MaxParts, lim.MaxItems, lim.MaxLength));
    }

    void TestParamSplitter() {
        using namespace NQueryData;
        {
            TRequestSplitLimits lims;
            lims.MaxParts = 0;
            lims.MaxLength = 0;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SplittableCount = 10;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
        }

        {
            TRequestSplitLimits lims;
            lims.MaxItems = 0;
            lims.MaxParts = 0;
            lims.MaxLength = 0;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SplittableCount = 10;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
        }

        {
            TRequestSplitLimits lims;
            lims.MaxItems = 1;
            lims.MaxParts = 10;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SplittableCount = 10;
            DoTestParamSplitter(10, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(10, m, lims);
        }

        {
            TRequestSplitLimits lims;
            lims.MaxParts = 1;
            lims.MaxLength = 50;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 100;
            DoTestParamSplitter(1, m, lims);
        }

        {
            TRequestSplitLimits lims;
            lims.MaxParts = 2;
            lims.MaxLength = 50;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 10;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(2, m, lims);
            m.SharedSize = 10;
            DoTestParamSplitter(2, m, lims);
        }

        {
            TRequestSplitLimits lims;
            lims.MaxParts = 3;
            lims.MaxLength = 50;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 10;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(2, m, lims);
            m.SharedSize = 10;
            DoTestParamSplitter(3, m, lims);
        }

        {
            TRequestSplitLimits lims;
            lims.MaxParts = 3;
            lims.MaxLength = 73;

            TRequestSplitMeasure m;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 10;
            DoTestParamSplitter(1, m, lims);
            m.SharedSize = 0;
            m.SplittableSize = 100;
            DoTestParamSplitter(2, m, lims);
            m.SharedSize = 10;
            DoTestParamSplitter(2, m, lims);
        }

        UNIT_ASSERT_VALUES_EQUAL(CountSize(NSc::TValue::FromJson("{"
                        "abcdefghij:null, o123456789:null, ABCDEFGHIJ:null"
                        "}").DictKeys()), 39u);

        UNIT_ASSERT_VALUES_EQUAL(VectorToString(ArrayToVector(NSc::TValue::FromJson("[a,b,c]")), ','),
                                 "a,b,c");
        UNIT_ASSERT_VALUES_EQUAL(VectorToString(NSc::TValue::FromJson("{a:null,b:null,c:null,d:null}").DictKeys(), ','),
                                 "a,b,c,d");
        UNIT_ASSERT_VALUES_EQUAL(VectorToStringSplit(NSc::TValue::FromJson("{a:null,b:null,c:null,d:null}").DictKeys(), ',', 1, 2),
                                 "c,d");
        UNIT_ASSERT_VALUES_EQUAL(VectorToStringSplit(NSc::TValue::FromJson("{a:null,b:null,c:null,d:null}").DictKeys(), ',', 0, 2),
                                 "a,b");
    }

    void DoTestCompressCgi(TStringBuf cgi) {
        using namespace NQueryData;
        UNIT_ASSERT_STRINGS_EQUAL(cgi, DecompressCgi(CompressCgi(cgi, CC_ZLIB), CC_ZLIB));
        UNIT_ASSERT_STRINGS_EQUAL(cgi, DecompressCgi(CompressCgi(cgi, CC_LZ4), CC_LZ4));
        UNIT_ASSERT_STRINGS_EQUAL(cgi, DecompressCgi(CompressCgi(cgi, CC_BASE64), CC_BASE64));
        UNIT_ASSERT_STRINGS_EQUAL(cgi, DecompressCgi(CompressCgi(cgi, CC_PLAIN), CC_PLAIN));
    }

    void TestCompressCgi() {
        using namespace NQueryData;
        DoTestCompressCgi(TStringBuf());
        DoTestCompressCgi("https://ru.wikipedia.org/wiki/%D0%A2%D0%B5%D1%81%D1%82");
        DoTestCompressCgi("Бобэоби пелись губы.\n"
                          "Вээоми пелись взоры,\n"
                          "Пиээо пелись брови,\n"
                          "Лииэй - пелся облик,\n"
                          "Гзи-гзи-гзэо пелась цепь.");
        UNIT_ASSERT_VALUES_EQUAL(CompressCgi("http://yandex.ru", CC_ZLIB), "eJzLKCkpsNLXr0zMS0mt0CsqBQAyVgX3");
        UNIT_ASSERT_VALUES_EQUAL(CompressCgi("http://yandex.ru", CC_BASE64), "aHR0cDovL3lhbmRleC5ydQ,,");
        UNIT_ASSERT_VALUES_EQUAL(CompressCgi("http://yandex.ru", CC_LZ4), "TFouNAEAAAAAgBAAAGh0dHA6Ly95YW5kZXgucnUAAAA,");
        UNIT_ASSERT_VALUES_EQUAL(CompressCgi("http://yandex.ru/?a=b", CC_PLAIN), "http://yandex.ru/%3Fa%3Db");

        UNIT_ASSERT_VALUES_EQUAL(DecompressCgi(
                "eNolyMsNgCAMANCzy9C7Y7iAqdCESvmkLbK-Jp5e8rL72AHWWuGe5sEVHxLY8v84hjC2SBYWF66oHnSCVRQB5Uhn7L2QGlyKLcFBqfbPF5FxIJU",
                CC_ZLIB), "http://www.just.travel/\thttp://appliances.wikimart.ru/small/rice_cookers/brand/Redmond/");

        UNIT_ASSERT_VALUES_EQUAL(DecompressCgi(
                "aHR0cDovL3d3dy5qdXN0LnRyYXZlbC8JaHR0cDovL2FwcGxpYW5jZXMud2lraW1hcnQucnUvc21hbGwvcmljZV9jb29rZXJzL2JyYW5kL1JlZG1vbmQv",
                CC_BASE64), "http://www.just.travel/\thttp://appliances.wikimart.ru/small/rice_cookers/brand/Redmond/");

        UNIT_ASSERT_VALUES_EQUAL(DecompressCgi(
                "TFouNAEAAAAAgFYAAfMJaHR0cDovL3d3dy5qdXN0LnRyYXZlbC8JGADwKWFwcGxpYW5jZXMud2lraW1hcnQucnUvc21hbGwvcmljZV9jb29rZXJzL2JyYW5kL1JlZG1vbmQvAAAA",
                CC_LZ4), "http://www.just.travel/\thttp://appliances.wikimart.ru/small/rice_cookers/brand/Redmond/");
    }

    void TestParseLists() {
        using namespace NQueryData;
        NSc::TValue v;
        const auto& vec = Split<TStringBuf>(",1,2,,3,", ',');
        v.GetArrayMutable().assign(vec.begin(), vec.end());
        NSc::NUt::AssertSchemeJson("['1','2','3']", v);
    }

    template <typename T>
    void DoTestList(TVector<T> l, TStringBuf ref, const TString& sep = ",", bool sort = true) {
        if (sort) {
            Sort(l.begin(), l.end());
        }
        UNIT_ASSERT_VALUES_EQUAL(JoinStrings(l.begin(), l.end(), sep), ref);
    }

    void DoTestSet(const TSet<TString>& l, TStringBuf ref, const TString& sep = ",") {
        UNIT_ASSERT_VALUES_EQUAL(JoinStrings(l.begin(), l.end(), sep), ref);
    }

    void DoTestCGI(const NSc::TValue& reqBase, bool enableDocItems, bool compress) {
        using namespace NQueryData;

        TStringBuf docItemsOff = "{"
                "DocIds:{ZABC:1,ZDEF:1,ZGHI:1,ZKLM:1,Z3D6A9F4FA7A3E4D7:1,Z2CF412508A0C9390:1}"
                ",SnipDocIds:{ZMDE:1,ZRTG:1,Z41927BBFE198B39C:1,ZC1E5B0BD2885B979:1}"
                ",SnipCategs:{'bash.im':1,'bar-owner.com':1}"
                ",Categs:{'yandex.ru':1,'facebook.com':1,'foo-owner.com':1}"
        "}";

        NSc::TValue reqRes = reqBase;

        if (!enableDocItems) {
            reqRes["DocItems"] = NSc::NUt::AssertFromJson(docItemsOff);
        }

        TRequestBuilderOptions opts;
        opts.SetEnableUrls(enableDocItems);
        opts.SetUrlsCompression(compress ? CC_ZLIB : CC_PLAIN);

        TRequestRec rec;
        AdaptQueryDataRequest(rec, reqBase);

        {
            NSc::TValue res;
            AdaptQueryDataRequest(res, rec);
            UNIT_ASSERT_JSON_EQ_JSON(res, reqBase);
        }

        UNIT_ASSERT_VALUES_EQUAL(rec.YandexTLD, "com.tr");
        UNIT_ASSERT_VALUES_EQUAL(rec.UserQuery, "test?");
        UNIT_ASSERT_VALUES_EQUAL(rec.DoppelNorm, "dtest?");
        UNIT_ASSERT_VALUES_EQUAL(rec.DoppelNormW, "dwtest?");
        UNIT_ASSERT_VALUES_EQUAL(rec.StrongNorm, "stest?");
        UNIT_ASSERT_VALUES_EQUAL(rec.UserId, "myid");
        UNIT_ASSERT_VALUES_EQUAL(rec.UserLogin, "velavokr");
        UNIT_ASSERT_VALUES_EQUAL(rec.UserLoginHash, "rkovalev");
        UNIT_ASSERT_VALUES_EQUAL(rec.SerpType, "smart");
        UNIT_ASSERT_VALUES_EQUAL(rec.UserIpMobileOp, "MTS");
        UNIT_ASSERT_VALUES_EQUAL(rec.UserIpMobileOpDebug, "MTS2");

        DoTestList(rec.UserRegions, "213,200,100", ",", false);
        DoTestList(rec.UserRegionsIpReg, "225,84", ",", false);
        DoTestList(rec.UserRegionsIpRegDebug, "12,11", ",", false);
        DoTestList(rec.DocItems.DocIds(), "ZABC,ZDEF,ZGHI,ZKLM");
        DoTestList(rec.DocItems.SnipDocIds(), "ZMDE,ZRTG");
        DoTestList(rec.DocItems.Categs(), "facebook.com,yandex.ru");
        DoTestList(rec.DocItems.SnipCategs(), "bash.im");
        DoTestSet(rec.FilterNamespaces, "bar,foo");
        DoTestSet(rec.FilterFiles, "bar.trie,foo.trie");
        DoTestSet(rec.SkipNamespaces, "ban,baz");
        DoTestSet(rec.SkipFiles, "ban.trie,baz.trie");

        UNIT_ASSERT_JSON_EQ_JSON(rec.StructKeys, "{ns1:[Key1,Key2],ns2:{foo:null,bar:null},ns3:baz}");

        {
            TVector<TString> vs;
            TRequestSplitLimits lims;
            lims.MaxParts = 2;
            TQuerySearchRequestBuilder(opts, lims).FormRequests(vs, *TRequestRec::FromJson(reqBase));

            UNIT_ASSERT_VALUES_EQUAL(vs.size(), 1u);

            if (enableDocItems) {
                if (compress) {
                    UNIT_ASSERT_VALUES_EQUAL(vs[0],
                        "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                        "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                        "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                        ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                        ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                        ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                        ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                        ";docids=ZABC%2CZDEF%2CZGHI%2CZKLM"
                        ";snipdocids=ZMDE%2CZRTG"
                        ";qd_categs=facebook.com%2Cyandex.ru"
                        ";qd_snipcategs=bash.im"
                        ";qd_urls:zlib:1=eJyrVkrLz9fNL89LLdJLzs_VT0ossk-0TVJLtk2xTrVNUzVyTlfOULJCVaWkg6arorIKQ00tAIM4HwM%2C"
                        ";qd_snipurls:zlib:1=eJyrVkpKLNLNL89LLdJLzs_VT0xKVrJCFVPSQVOTlp-PoaYWAKg_GVk%2C"
                        ";&");
                } else {
                    UNIT_ASSERT_VALUES_EQUAL(vs[0],
                        "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                        "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                        "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                        ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                        ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                        ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                        ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                        ";docids=ZABC%2CZDEF%2CZGHI%2CZKLM"
                        ";snipdocids=ZMDE%2CZRTG"
                        ";qd_categs=facebook.com%2Cyandex.ru"
                        ";qd_snipcategs=bash.im"
                        ";qd_urls:1=%7B"
                            "%22foo-owner.com/bar%253Fa%253Db%2526c%253Dd%253Be%253Df%25252Cg%2523h%22%3A%22foo-owner.com%22"
                            "%2C%22foo-owner.com/xyz%22%3A%22foo-owner.com%22%7D"
                        ";qd_snipurls:1=%7B"
                            "%22bar-owner.com/abc%22%3A%22bar-owner.com%22"
                            "%2C%22bar-owner.com/foo%22%3A%22bar-owner.com%22%7D"
                        ";&");
                }
            } else {
                    UNIT_ASSERT_VALUES_EQUAL(vs[0],
                        "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                        "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                        "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                        ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                        ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                        ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                        ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                        ";docids=ZABC%2CZDEF%2CZGHI%2CZKLM%2CZ3D6A9F4FA7A3E4D7%2CZ2CF412508A0C9390"
                        ";snipdocids=ZMDE%2CZRTG%2CZC1E5B0BD2885B979%2CZ41927BBFE198B39C"
                        ";qd_url=facebook.com%2Cyandex.ru%2Cfoo-owner.com%2Cfoo-owner.com"
                        ";qd_snipcategs=bash.im%2Cbar-owner.com%2Cbar-owner.com"
                        ";&"
                    );
            }

            TRequestRec parsed;
            TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(vs[0]));
            UNIT_ASSERT_JSON_EQ_JSON(parsed.ToJson(), reqRes);
        }

        if (compress) {
            return;
        }

        {
            TVector<TString> vs;
            TRequestSplitLimits lims;
            lims.MaxItems = 9;
            lims.MaxParts = 3;
            lims.MaxLength = 900;

            TQuerySearchRequestBuilder(opts, lims).FormRequests(vs, *TRequestRec::FromJson(reqBase));

            UNIT_ASSERT_VALUES_EQUAL(vs.size(), 2u);
            if (enableDocItems) {
                UNIT_ASSERT_VALUES_EQUAL(vs[0],
                    "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                    "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                    "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                    ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                    ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                    ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                    ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                    ";docids=ZABC%2CZDEF"
                    ";snipdocids=ZMDE"
                    ";qd_categs=facebook.com"
                    ";qd_snipcategs=bash.im"
                    ";qd_urls:1=%7B%22foo-owner.com/bar%253Fa%253Db%2526c%253Dd%253Be%253Df%25252Cg%2523h%22%3A%22foo-owner.com%22%7D"
                    ";qd_snipurls:1=%7B%22bar-owner.com/abc%22%3A%22bar-owner.com%22%7D;&");
                UNIT_ASSERT_VALUES_EQUAL(vs[1],
                    "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                    "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                    "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                    ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                    ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                    ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                    ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                    ";docids=ZGHI%2CZKLM"
                    ";snipdocids=ZRTG"
                    ";qd_categs=yandex.ru"
                    ";qd_urls:1=%7B%22foo-owner.com/xyz%22%3A%22foo-owner.com%22%7D"
                    ";qd_snipurls:1=%7B%22bar-owner.com/foo%22%3A%22bar-owner.com%22%7D;&");
            } else {
                UNIT_ASSERT_VALUES_EQUAL(vs[0],
                    "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                    "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                    "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                    ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                    ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                    ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                    ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                    ";docids=ZABC%2CZDEF%2CZGHI"
                    ";snipdocids=ZMDE%2CZRTG"
                    ";qd_url=facebook.com%2Cyandex.ru"
                    ";qd_snipcategs=bash.im"
                    ";&");
                UNIT_ASSERT_VALUES_EQUAL(vs[1],
                    "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                    "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                    "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                    ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                    ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                    ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                    ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                    ";docids=ZKLM%2CZ3D6A9F4FA7A3E4D7%2CZ2CF412508A0C9390"
                    ";snipdocids=ZC1E5B0BD2885B979%2CZ41927BBFE198B39C"
                    ";qd_url=foo-owner.com%2Cfoo-owner.com"
                    ";qd_snipcategs=bar-owner.com%2Cbar-owner.com"
                    ";&");
            }

            TRequestRec parsed;
            TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(vs[0]));
            TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(vs[1]));
            UNIT_ASSERT_JSON_EQ_JSON(parsed.ToJson(), reqRes);

        }

        if (!enableDocItems) {
            TVector<TString> vs;
            TRequestSplitLimits lims;
            lims.MaxItems = 2;
            lims.MaxParts = 3;
            lims.MaxLength = 7500;

            TQuerySearchRequestBuilder(opts, lims).FormRequests(vs, *TRequestRec::FromJson(reqBase));

            UNIT_ASSERT_VALUES_EQUAL(vs.size(), 3u);
            UNIT_ASSERT_VALUES_EQUAL(vs[0],
                "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                ";docids=ZABC%2CZDEF"
                ";snipdocids=ZMDE"
                ";qd_url=facebook.com"
                ";qd_snipcategs=bash.im"
                ";&");
            UNIT_ASSERT_VALUES_EQUAL(vs[1],
                "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                ";docids=ZGHI%2CZKLM"
                ";snipdocids=ZRTG"
                ";qd_url=yandex.ru"
                ";qd_snipcategs=bar-owner.com"
                ";&");
            UNIT_ASSERT_VALUES_EQUAL(vs[2],
                "text=test%3F&reqid=blablabla&tld=com.tr&noqtree=1"
                "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                "&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie"
                ";device=smart;is_mobile=1;yandexuid=myid;login=velavokr;loginhash=rkovalev"
                ";all_regions=213%2C200%2C100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2"
                ";qd_struct_keys=%7Bns1%3A%5BKey1%2CKey2%5D%2Cns2%3A%7Bbar%3Anull%2Cfoo%3Anull%7D%2Cns3%3Abaz%7D"
                ";docids=Z3D6A9F4FA7A3E4D7%2CZ2CF412508A0C9390"
                ";snipdocids=ZC1E5B0BD2885B979%2CZ41927BBFE198B39C"
                ";qd_url=foo-owner.com%2Cfoo-owner.com"
                ";qd_snipcategs=bar-owner.com"
                ";&");

            TRequestRec parsed;
            TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(vs[0]));
            TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(vs[1]));
            TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(vs[2]));
            UNIT_ASSERT_JSON_EQ_JSON(parsed.ToJson(), reqRes);
        }

        if (!enableDocItems) {
            NSc::TValue tmp = reqBase.Clone();
            tmp.Delete("FilterNamespaces");
            tmp.Delete("FilterFiles");
            tmp.Delete("SkipNamespaces");
            tmp.Delete("SkipFiles");
            tmp["DocItems"].Delete("Urls");
            tmp["DocItems"].Delete("SnipUrls");

            TCgiParameters pars;
            pars.Scan("text=1&upoint=10.1,11.1&lr=213"
                            "&qtree=cHicdZC9S8NQFMXPvQn2GTsERShvkjg0OhWHIk7iYgeH0kkySbEYF5G2gzjVilBxEdwEQRShk9RKQURE"
                            "cHNKdmf_iiK-vKYftPimwz3v3MP9WTkrKWAjJRbgcgazZrVUqUo4WMYK1ixhKg_KwzpyyGMbO1N-N9sgXBKuCXe"
                            "kA21CBqu08U2CbEg9c-B2sxnKUf6KVQw-lTct7diUIjcKoEANjx8ei0KFopa5oqUUpytKG8VpmyX1JSSlI-njiM"
                            "uLk4tqHoZr9KetyU_nHt8-e_Q0Xlja1S0zqtDYP9iLK424Pdp2zIK9VxZiHurJJluFMXIirAed8CSsS1pyyP2Pn"
                            "9_6-TVH-A1ifYafPYaDueJICiPn7_sYb8gauDanEF9nND6-IpoeNyeIBi19U0-_jeiXWLPk8Gyog47WSZukGbwH"
                            "7dihkXSUOB3Ow4sYFSlU5CcOEzV9pGNGlFwMJ39kRX9o"
                            "&user_request=test%3F&tld=com.tr&lr=213&reqid=blablabla"
                            "&relev=uil=ru;norm=stest%3F;dnorm=dtest%3F;dnorm_w=dwtest%3F;"
                            "&rearr=yandexuid=myid;login=velavokr;loginhash=rkovalev;device=smart;is_mobile=1"
                            "&rearr=all_regions=,213,,200,100;urcntr=225%2C84;urcntr_debug=12%2C11;gsmop=MTS;gsmop_debug=MTS2;"
                            "qd_struct_keys={\"ns1\":[\"Key1\",\"Key2\"]};"
                            "qd_struct_keys={\"ns2\":{\"foo\":null,\"bar\":null}};"
                            "qd_struct_keys={\"ns3\":\"baz\"};"
                            "docids=ZABC,ZDEF;"
                            "snipdocids=ZMDE,ZRTG;docids=ZGHI;docids=ZKLM;;qd_url=yandex.ru,,facebook.com;qd_snipcategs=bash.im;"
                    );

            TRequestRec res;
            TQuerySearchRequestBuilder().ParseFormedRequest(res, pars);
            UNIT_ASSERT_JSON_EQ_JSON(res.ToJson(), tmp);
        }

        {
            TCgiParameters pars;
            pars.InsertUnescaped("qsreq:json/zlib:1", CompressCgi(NJson::CompactifyJson(reqBase.ToJson()), CC_ZLIB));
            pars.InsertUnescaped("text", "xxx");
            TRequestRec res;
            TQuerySearchRequestBuilder().ParseFormedRequest(res, pars);
            UNIT_ASSERT_JSON_EQ_JSON(res.ToJson(), reqBase);
        }

        {
            TCgiParameters pars;
            pars.InsertUnescaped("qsreq:json:1", NJson::CompactifyJson(reqBase.ToJson(),true));
            pars.InsertUnescaped("text", "xxx");
            pars.InsertUnescaped("qsreq:json/zlib:1",
                                 CompressCgi(
                                         NJson::CompactifyJson(NSc::NUt::AssertFromJson("{UserQuery:yyy}").ToJson(true)),
                                         CC_ZLIB));
            TRequestRec res;
            TQuerySearchRequestBuilder().ParseFormedRequest(res, pars);
            UNIT_ASSERT_JSON_EQ_JSON(res.ToJson(), reqBase);
        }

        {
            TCgiParameters pars;
            pars.Scan("text=1&tld=ru&rearr=qd_filter_namespaces=bar%2Cfoo;qd_filter_files=bar.trie%2Cfoo.trie"
                    ";qd_skip_namespaces=ban%2Cbaz;qd_skip_files=ban.trie%2Cbaz.trie;");

            TRequestRec res;
            TQuerySearchRequestBuilder().ParseFormedRequest(res, pars);
            UNIT_ASSERT_JSON_EQ_JSON(res.ToJson(), "{"
                    "UserQuery:'1',YandexTLD:ru,IgnoreText:0"
                    ",FilterNamespaces:{bar:1,foo:1},FilterFiles:{'bar.trie':1,'foo.trie':1}"
                    ",SkipNamespaces:{ban:1,baz:1},SkipFiles:{'ban.trie':1,'baz.trie':1}"
                    "}");
        }

        {
            TCgiParameters pars;
            pars.Scan("text=1&tld=ru&rearr=qd_ignore_text=1");

            TRequestRec res;
            TQuerySearchRequestBuilder().ParseFormedRequest(res, pars);
            UNIT_ASSERT_JSON_EQ_JSON(res.ToJson(), "{"
                    "IgnoreText:1,YandexTLD:ru"
                    "}");
        }
    }

    // TODO: refactor this shit
    void TestCGI() {
        TStringBuf json = "{"
                        " YandexTLD:'com.tr'"
                        ",DoppelNorm:'dtest?'"
                        ",DoppelNormW:'dwtest?'"
                        ",StrongNorm:'stest?'"
                        ",UserQuery:'test?'"
                        ",FilterNamespaces:{bar:1,foo:1}"
                        ",FilterFiles:{'bar.trie':1,'foo.trie':1}"
                        ",SkipNamespaces:{ban:1,baz:1}"
                        ",SkipFiles:{'ban.trie':1,'baz.trie':1}"
                        ",IgnoreText:0"
                        ",UserId:myid"
                        ",UserLogin:velavokr"
                        ",UserLoginHash:rkovalev"
                        ",UserRegions:['213','200','100']"
                        ",SerpType:smart"
                        ",DocItems:{"
                             "DocIds:{ZABC:1,ZDEF:1,ZGHI:1,ZKLM:1}"
                            ",SnipDocIds:{ZMDE:1,ZRTG:1}"
                            ",SnipCategs:{'bash.im':1}"
                            ",Categs:{'yandex.ru':1,'facebook.com':1}"
                            ",Urls:{"
                                 "'foo-owner.com/bar?a=b&c=d;e=f%2Cg#h':'foo-owner.com'" // Z3D6A9F4FA7A3E4D7
                                ",'foo-owner.com/xyz':'foo-owner.com'" // Z2CF412508A0C9390
                            "}"
                            ",SnipUrls:{"
                                 "'bar-owner.com/foo':'bar-owner.com'," // Z41927BBFE198B39C
                                 "'bar-owner.com/abc':'bar-owner.com'" // ZC1E5B0BD2885B979
                            "}"
                        "}"
                        ",StructKeys:{ns1:[Key1,Key2],ns2:{foo:null,bar:null},ns3:baz}"
                        ",ReqId:'blablabla'"
                        ",UILang:ru"
                        ",UserIpMobileOp:MTS"
                        ",UserIpMobileOpDebug:MTS2"
                        ",UserRegionsIpReg:['225','84']"
                        ",UserRegionsIpRegDebug:['12','11']"
                        "}";
        {
            NSc::TValue req = NSc::NUt::AssertFromJson(json);
            DoTestCGI(req, false, false);
            DoTestCGI(req, true, false);
            DoTestCGI(req, true, true);
        }
    }
    void TestJsonArrayQdUrls() {
        using namespace NQueryData;
        TString qs = ("rearr=qd_urls:base64:1=WyJmb28tb3duZXIuY29tL2JhciIsImZhY2Vib29rLmNvbS9teXBhZ2UiXQo");
        TRequestRec parsed;
        TQuerySearchRequestBuilder().ParseFormedRequest(parsed, TCgiParameters(qs));
        DoTestList(parsed.DocItems.AllCategsNoCopy(), "facebook.com,foo-owner.com");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQueryDataCgiTest)

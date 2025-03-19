#include <kernel/querydata/request/qd_genrequests.h>

#include <kernel/querydata/idl/querydata_structs.pb.h>

#include <library/cpp/langmask/langmask.h>
#include <library/cpp/scheme/ut_utils/scheme_ut_utils.h>
#include <library/cpp/string_utils/relaxed_escaper/relaxed_escaper.h>
#include <library/cpp/testing/unittest/registar.h>

class TQueryDataRequestTest : public TTestBase {
    UNIT_TEST_SUITE(TQueryDataRequestTest)
        UNIT_TEST(TestNormalizations)
        UNIT_TEST(TestRawKeys)
    UNIT_TEST_SUITE_END();

    struct TNormResult {
        TString Result;
        ui64 Inferiority = 0;

        TNormResult() = default;

        TNormResult(const TString& res, ui64 inf)
            : Result(res)
            , Inferiority(inf)
        {}
    };

    struct TNormTest {
        TVector<TStringBuf> NormName;
        TString NameSpace;
        TVector<TNormResult> Result;

        TNormTest() = default;

        TNormTest(const TVector<TStringBuf>& nm, const TString& ns, const TVector<TNormResult>& res)
            : NormName(nm)
            , NameSpace(ns)
            , Result(res)
        {}
    };

    void DoMakeNormalizations(NQueryData::TRawKeys& t, const NQueryData::TRequestRec& r,
                              const TVector<TStringBuf>& norm, TStringBuf nspace) {
        using namespace NQueryData;
        t.ClearTuples();
        NQueryData::NormalizeQuery(t, r, TStringBufs(norm.begin(), norm.end()), nspace);
    }

    void AssertNormalization(const NQueryData::TRawKeys& t, const TVector<TNormResult>& res) {
        TStringBuilder mess;

        for (ui32 i = 0; i < res.size(); ++i) {
            mess << (i ? "," : "") << NEscJ::EscapeJ<false>(res[i].Result);

        }

        mess << " != ";

        for (ui32 i = 0; i < t.Tuples.size(); ++i) {
            mess << (i ? "," : "") << NEscJ::EscapeJ<false>(t.Tuples[i].AsStrBuf());
        }

        UNIT_ASSERT_VALUES_EQUAL_C(t.Tuples.size(), res.size(), mess);

        for (ui32 i = 0; i < res.size(); ++i) {
            UNIT_ASSERT_STRINGS_EQUAL_C(t.Tuples[i].AsStrBuf(), res[i].Result, (mess << " @" << i));
            UNIT_ASSERT_VALUES_EQUAL_C(t.Tuples[i].GetInferiority(), res[i].Inferiority, (mess << " @" << i));
        }
    }

    void DoTestNormalizations(const NQueryData::TRequestRec& r, const TVector<TNormTest>& norms) {
        using namespace NQueryData;
        TRawKeys t;
        for (const auto& norm : norms) {
            DoMakeNormalizations(t, r, norm.NormName, norm.NameSpace);
            AssertNormalization(t, norm.Result);
        }
    }

    void TestNormalizations() {
        using namespace NQueryData;

        TRequestRec r;

        {
            TVector<TNormTest> norms {
                            TNormTest{{GetNormalizationNameFromType(KT_QUERY_EXACT)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_QUERY_LOWERCASE)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_QUERY_SIMPLE)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_QUERY_DOPPEL)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_QUERY_STRONG)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_QUERY_DOPPEL_TOKEN)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_QUERY_DOPPEL_PAIR)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_USER_ID)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_USER_LOGIN)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_USER_LOGIN_HASH)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_USER_REGION)}, "", {{"0", 0} }}
                          , TNormTest{{GetNormalizationNameFromType(KT_USER_REGION_IPREG)}, "", {{"0", 0} }}
                          , TNormTest{{GetNormalizationNameFromType(KT_USER_IP_TYPE)}, "", {{"*", 0} }}
                          , TNormTest{{GetNormalizationNameFromType(KT_DOCID)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_SNIPDOCID)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_CATEG)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_SNIPCATEG)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_CATEG_URL)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_SNIPCATEG_URL)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_BINARYKEY)}, "", {}}
                          , TNormTest{{GetNormalizationNameFromType(KT_SERP_TLD)}, "", {{"*", 0} }}
                          , TNormTest{{GetNormalizationNameFromType(KT_SERP_UIL)}, "", {{"*", 0} }}
                          , TNormTest{{GetNormalizationNameFromType(KT_SERP_DEVICE)}, "", {{"*", 0} }}
            };
            DoTestNormalizations(r, norms);
        }

        r.StructKeys = NSc::TValue::FromJson("{"
                            "nspace0:'',"
                            "nspaceS:ab,"
                            "nspaceN:13,"
                            "nspaceK:{a:null,b:null},"
                            "nspaceA:[pr0,pr1,pr2]"
                        "}");
        r.UserQuery = "Ёжик на помойке жрёт шины";
        r.DoppelNorm = "ежиха жрать помойка шина";
        r.DoppelNormW = "ёжик на помой жрёт шины";
        r.StrongNorm = "ежик помойке жрет шины";
        r.UserId = "y100500";
        r.UserIpMobileOp = "0";
        r.UserRegionsIpReg.push_back("321");
        r.UserRegionsIpReg.push_back("654");
        r.UserLogin = "velavokr";
        r.UserLoginHash = "rkovalev";
        r.DocItems.AddDocId("1234");
        r.DocItems.AddDocId("5678");
        r.DocItems.AddDocId("9ABC");
        r.DocItems.AddDocId("DEF0");
        r.DocItems.AddDocId("Zfoocom");
        r.DocItems.AddSnipDocId("XXX");
        r.DocItems.AddSnipDocId("YYY");
        r.DocItems.AddSnipDocId("ZZZ");
        r.DocItems.AddSnipDocId("Zbarcom");
        r.UserRegions.push_back("123");
        r.UserRegions.push_back("456");
        r.UserRegions.push_back("789");
        r.YandexTLD = "com.tr";
        r.UILang = "tr";
        r.DocItems.AddCateg("yandex.ru");
        r.DocItems.AddCateg("bash.im");
        r.DocItems.AddCateg("bbar.com");
        r.DocItems.AddSnipCateg("google.ru");
        r.DocItems.AddSnipCateg("google.com");
        r.DocItems.AddSnipCateg("ffoo.com");
        r.BinaryKeys.push_back(TString{TStringBuf("\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF"sv)});
        r.BinaryKeys.push_back(TString{TStringBuf("\xF\xE\xD\xC\xB\xA\x9\x8\x7\x6\x5\x4\x3\x2\x1\x0"sv)});
        r.SerpType = "smart";
        r.DocItems.AddUrl("Foo.com/xxx", "foo.com");
        r.DocItems.AddSnipUrl("Bar.com/yyy", "bar.com");

        {
            TVector<TNormTest> norms {
                  TNormTest{{ GetNormalizationNameFromType(KT_QUERY_EXACT)   }, "", { {"Ёжик на помойке жрёт шины",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_QUERY_LOWERCASE) }, "", { {"ёжик на помойке жрёт шины",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_QUERY_SIMPLE) }, "", { {"ежик на помойке жрет шины",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_QUERY_DOPPEL) }, "", { {"ежиха жрать помойка шина",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_QUERY_STRONG) }, "", { {"ежик помойке жрет шины",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_QUERY_DOPPEL_TOKEN)}, "", {
                                {"ежиха",0}, {"жрать",0}, {"помойка",0}, {"шина",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_QUERY_DOPPEL_PAIR)}, "", {
                                {"ежиха жрать",0}, {"ежиха помойка",0}, {"ежиха шина",0}, {"ежиха",0}
                              , {"жрать помойка",0}, {"жрать шина",0}, {"жрать",0}
                              , {"помойка шина", 0}, {"помойка",0}
                              , {"шина",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_USER_ID)    }, "", { {"y100500",0}}}
                , TNormTest{{ GetNormalizationNameFromType(KT_USER_LOGIN)    }, "", { {"velavokr",0}}}
                , TNormTest{{ GetNormalizationNameFromType(KT_USER_LOGIN_HASH)    }, "", { {"rkovalev",0}}}
                , TNormTest{{ GetNormalizationNameFromType(KT_USER_REGION)   }, "", { {"123",0}, {"456",1}, {"789",2}, {"0",3} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_USER_REGION_IPREG)   }, "", { {"321",0}, {"654",1}, {"0",2} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_USER_IP_TYPE)}, "", {{"*",0}}}
                , TNormTest{{ GetNormalizationNameFromType(KT_DOCID)  }, "", {
                                {"1234",0}, {"5678",0}, {"9ABC",0}, {"DEF0",0}, {"Zfoocom", 0}, {"ZC369B2F755D5E874", 0},
                                {"XXX",0}, {"YYY",0}, {"ZZZ", 0}, {"Zbarcom", 0}, {"Z4B9DCB692CAFBF31", 0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_SNIPDOCID)  }, "", {
                                {"XXX", 0}, {"YYY", 0}, {"ZZZ", 0}, {"Zbarcom", 0}, {"Z4B9DCB692CAFBF31", 0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_CATEG)  }, "", {
                                {"yandex.ru",0}, {"bash.im", 0}, {"bbar.com", 0}, {"foo.com", 0},
                                {"google.ru",0}, {"google.com",0}, {"ffoo.com", 0}, {"bar.com", 0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_SNIPCATEG)  }, "", {
                                {"google.ru",0}, {"google.com", 0}, {"ffoo.com",0}, {"bar.com", 0},}}
                , TNormTest{{ GetNormalizationNameFromType(KT_CATEG_URL)  }, "", {
                                {"foo.com Foo.com/xxx", 0}, {"bar.com Bar.com/yyy", 0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_SNIPCATEG_URL)  }, "", {
                                {"bar.com Bar.com/yyy", 0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_SERP_TLD)    }, "", { {"com.tr",0}, {"*",1} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_SERP_UIL)    }, "", { {"tr",0}, {"*",1} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_SERP_DEVICE)}, "", {{SERP_TYPE_SMART,0},{SERP_TYPE_MOBILE,1},{"*",2}}}
                , TNormTest{{ GetNormalizationNameFromType(KT_BINARYKEY)  }, "", {
                                {TString{TStringBuf("\x0\x1\x2\x3\x4\x5\x6\x7\x8\x9\xA\xB\xC\xD\xE\xF"sv)}, 0}
                              , {TString{TStringBuf("\xF\xE\xD\xC\xB\xA\x9\x8\x7\x6\x5\x4\x3\x2\x1\x0"sv)}, 0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_STRUCTKEY)}, "nspace0", { }}
                , TNormTest{{ GetNormalizationNameFromType(KT_STRUCTKEY)}, "nspaceS", { {"ab",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_STRUCTKEY)}, "nspaceN", { {"13",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_STRUCTKEY)}, "nspaceK", { {"a",0}, {"b",0} }}
                , TNormTest{{ GetNormalizationNameFromType(KT_STRUCTKEY)}, "nspaceA", { {"pr0",0}, {"pr1",1}, {"pr2",2} }}
            };

            DoTestNormalizations(r, norms);
        }

        r.UserIpMobileOp = "MTS";

        {
            TVector<TNormTest> norms {
                            TNormTest{{ GetNormalizationNameFromType(KT_USER_IP_TYPE)}, "", {{MOBILE_IP_ANY, 0}, {"*",1}}}
            };
            DoTestNormalizations(r, norms);
        }

        r.DoppelNorm = "";
        r.DoppelNormW = "";

        {
            TVector<TNormTest> norms {
                              TNormTest{{ GetNormalizationNameFromType(KT_QUERY_DOPPEL_TOKEN)
                               , GetNormalizationNameFromType(KT_USER_REGION)
                               , GetNormalizationNameFromType(KT_SERP_TLD)
                              }, "", {
                                 {"помойка\t123\tcom.tr",0}
                               , {"помойка\t123\t*",1}
                               , {"помойка\t456\tcom.tr",1 << 8}
                               , {"помойка\t456\t*",1 + (1 << 8)}
                               , {"помойка\t789\tcom.tr",2 << 8}
                               , {"помойка\t789\t*",1 + (2 << 8)}
                               , {"помойка\t0\tcom.tr",3 << 8}
                               , {"помойка\t0\t*",1 + (3 << 8)}
                               , {"ежик\t123\tcom.tr",0}
                               , {"ежик\t123\t*",1}
                               , {"ежик\t456\tcom.tr",1 << 8}
                               , {"ежик\t456\t*",1 + (1 << 8)}
                               , {"ежик\t789\tcom.tr",2 << 8}
                               , {"ежик\t789\t*",1 + (2 << 8)}
                               , {"ежик\t0\tcom.tr",3 << 8}
                               , {"ежик\t0\t*",1 + (3 << 8)}
                               , {"жрать\t123\tcom.tr",0}
                               , {"жрать\t123\t*",1}
                               , {"жрать\t456\tcom.tr",1 << 8}
                               , {"жрать\t456\t*",1 + (1 << 8)}
                               , {"жрать\t789\tcom.tr",2 << 8}
                               , {"жрать\t789\t*",1 + (2 << 8)}
                               , {"жрать\t0\tcom.tr",3 << 8}
                               , {"жрать\t0\t*",1 + (3 << 8)}
                               , {"шина\t123\tcom.tr",0}
                               , {"шина\t123\t*",1}
                               , {"шина\t456\tcom.tr",1 << 8}
                               , {"шина\t456\t*",1 + (1 << 8)}
                               , {"шина\t789\tcom.tr",2 << 8}
                               , {"шина\t789\t*",1 + (2 << 8)}
                               , {"шина\t0\tcom.tr",3 << 8}
                               , {"шина\t0\t*",1 + (3 << 8)}
                              }}

            };
            DoTestNormalizations(r, norms);
        }
    }

    void TestRawKeys() {
        using namespace NQueryData;
        TRawKeys keys;

        for (int i = 0; i < KT_COUNT; ++i)
            keys.GetSubkeysMutable(i).push_back(GetNormalizationNameFromType(i));

        for (int i = -1; i > FAKE_KT_COUNT; --i)
            keys.GetSubkeysMutable(i).push_back(GetNormalizationNameFromType(i));

        for (int i = 0; i < KT_COUNT; ++i) {
            if (KT_STRUCTKEY == i) {
                UNIT_ASSERT_VALUES_EQUAL(3u, keys.GetSubkeys(i).size());
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(KT_STRUCTKEY), keys.GetSubkeys(i)[0]);
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(FAKE_KT_STRUCTKEY_ANY), keys.GetSubkeys(i)[1]);
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(FAKE_KT_STRUCTKEY_ORDERED), keys.GetSubkeys(i)[2]);
            } else {
                UNIT_ASSERT_VALUES_EQUAL(1u, keys.GetSubkeys(i).size());
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(i), keys.GetSubkeys(i).back());
            }
        }

        for (int i = -1; i > FAKE_KT_COUNT; --i) {
            if (FAKE_KT_STRUCTKEY_ANY == i || FAKE_KT_STRUCTKEY_ORDERED == i) {
                UNIT_ASSERT_VALUES_EQUAL(3u, keys.GetSubkeys(i).size());
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(KT_STRUCTKEY), keys.GetSubkeys(i)[0]);
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(FAKE_KT_STRUCTKEY_ANY), keys.GetSubkeys(i)[1]);
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(FAKE_KT_STRUCTKEY_ORDERED), keys.GetSubkeys(i)[2]);
            } else {
                UNIT_ASSERT_VALUES_EQUAL(1u, keys.GetSubkeys(i).size());
                UNIT_ASSERT_STRINGS_EQUAL(GetNormalizationNameFromType(i), keys.GetSubkeys(i).back());
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQueryDataRequestTest)

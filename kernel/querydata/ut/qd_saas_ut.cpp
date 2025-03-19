#include <kernel/querydata/saas/qd_saas_key.h>
#include <kernel/querydata/saas/qd_saas_kv_key.h>
#include <kernel/querydata/saas/qd_saas_trie_key.h>
#include <kernel/querydata/saas/qd_saas_key_transform.h>
#include <kernel/querydata/saas/qd_saas_request.h>
#include <kernel/querydata/saas/qd_saas_response.h>
#include <kernel/querydata/cgi/qd_request.h>
#include <kernel/querydata/client/qd_client_utils.h>
#include <kernel/querydata/idl/querydata_structs_client.pb.h>
#include <kernel/urlid/url2docid.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/split.h>
#include <util/string/join.h>
#include <library/cpp/string_utils/base64/base64.h>

class TSaaSQuerySearchConv : public NUnitTest::TTestBase {
    UNIT_TEST_SUITE(TSaaSQuerySearchConv)
        UNIT_TEST(TestStringReprKV)
        UNIT_TEST(TestStringReprTrie)
        UNIT_TEST(TestKeyTypeStringRepr)
        UNIT_TEST(TestKeyValidation)
        UNIT_TEST(TestKeyGeneration)
        UNIT_TEST(TestUrlNormalization)
        UNIT_TEST(TestRequestRecUrls)
        UNIT_TEST(TestRequestRecFromQD)
        UNIT_TEST(TestResponseToQD)
    UNIT_TEST_SUITE_END();

    template <class T>
    static void CheckStr(const T& k, const TString& v) {
        UNIT_ASSERT_VALUES_EQUAL(ToString(k), v);
        UNIT_ASSERT_VALUES_EQUAL(FromString<T>(v), k);
    }

    void TestStringReprKV() {
        using namespace NQueryDataSaaS;
        CheckStr(TSaaSKVKey(), "00000000000000000000000000000000");
        CheckStr(TSaaSKVKey{1, 2}, "01000000000000000200000000000000");
        CheckStr(TSaaSKVKey::From(SST_IN_KEY_URL, "http://kernel.org/"), "B7C13C32D0F9025BB9B8F4CCA40F3927");
        CheckStr(TSaaSKVKey::From(SST_IN_KEY_URL, "http://kernel.org/").Append(SST_IN_KEY_USER_REGION, "225"), "971B551438B8A1E77E0BAC52A0A9AFFF");
    }

    void TestStringReprTrie() {
        using namespace NQueryDataSaaS;
        CheckStr(TSaaSTrieKey(), "");
        CheckStr(TSaaSTrieKey::From(SST_IN_KEY_URL, "http://kernel.org/"), ".7\thttp://kernel.org/");
        CheckStr(TSaaSTrieKey::From(SST_IN_KEY_URL, "http://kernel.org/").Append(SST_IN_KEY_USER_REGION, "225"), ".7.5\thttp://kernel.org/\t225");
    }

    template <class T>
    static void CheckStrVec(const T& k, const TString& v) {
        UNIT_ASSERT_VALUES_EQUAL(ToString(k), v);
        UNIT_ASSERT_VALUES_EQUAL(FromString<T>(v), k);
    }

    static void CheckStrTypeVec(const NQueryDataSaaS::TSaaSKeyType& k, const TString& v) {
        UNIT_ASSERT_VALUES_EQUAL(ToString(k), v);
        UNIT_ASSERT_VALUES_EQUAL(FromString<NQueryDataSaaS::TSaaSKeyType>(v), k);
    }

    void TestKeyTypeStringRepr() {
        using namespace NQueryDataSaaS;
        CheckStrTypeVec({SST_IN_KEY_QUERY_STRONG}, "QueryStrong");
        CheckStrTypeVec({SST_IN_KEY_QUERY_STRONG, SST_IN_KEY_OWNER, SST_IN_KEY_URL, SST_IN_KEY_URL_MASK}, "QueryStrong-Owner-Url-UrlMask");
    }

    void TestKeyValidation() {
        using namespace NQueryDataSaaS;
        UNIT_ASSERT(!KeyTypeIsValid({}));
        for (int i = 1 + SST_INVALID; i < SST_IN_KEY_COUNT; ++i) {
            bool valid1 = KeyTypeIsValid({(ESaaSSubkeyType)(i)});
            UNIT_ASSERT(valid1);

            for (int j = 1 + SST_INVALID; j < SST_IN_KEY_COUNT; ++j) {
                bool valid2 = KeyTypeIsValid({(ESaaSSubkeyType)(i), (ESaaSSubkeyType)(j)});
                UNIT_ASSERT_VALUES_EQUAL(valid2, i != j);

                for (int k = 1 + SST_INVALID; k < SST_IN_KEY_COUNT; ++k) {
                    bool valid3 = KeyTypeIsValid({(ESaaSSubkeyType)(i), (ESaaSSubkeyType)(j), (ESaaSSubkeyType)(k)});
                    UNIT_ASSERT_VALUES_EQUAL(valid3, i != j && i != k && j != k);
                }
            }
        }
    }

    // Реализация в тестах похожа на тестируемую. На самом деле, изначально тестируемый код был реализован иначе и хуже,
    // а затем переписан по мотивам более удачной реализации из тестов.
    struct TCounter {
        TCounter(const TVector<ui32>& sizes)
            : Sizes(sizes)
            , Counts(sizes.size())
        {}

        size_t Size() const {
            return Sizes.size();
        }

        bool IsValid() const {
            if (!Counts) {
                return false;
            }
            for (size_t i = 0; i < Counts.size(); ++i) {
                if (Counts[i] >= Sizes[i]) {
                    return false;
                }
            }
            return true;
        }

        bool Next() {
            if (!IsValid()) {
                return false;
            }

            ui32 i = 0;
            size_t sz = Size();
            while (i < sz) {
                const auto currSz = Sizes[i];
                auto& curr = Counts[i];
                auto next = curr + 1;
                if (currSz > next || i + 1 == sz) {
                    curr = next;
                    break;
                } else if (currSz == next) {
                    curr = 0;
                    i += 1;
                } else {
                    Y_FAIL("cannot be");
                }
            }

            return IsValid();
        }

        const TVector<ui32>& GetCurrent() const {
            return Counts;
        }

    private:
        const TVector<ui32> Sizes;
        TVector<ui32> Counts;
    };


    struct TAllSubkeys {
        using TSubkey = NQueryDataSaaS::TSaaSKVKey;
        TVector<TSubkey> HashQueryStrongVec;
        TVector<TSubkey> HashQueryDoppelVec;
        TVector<TSubkey> HashUserRegionVec;
        TVector<TSubkey> HashUserIdVec;
        TVector<TSubkey> HashUserLoginVec;
        TVector<TSubkey> HashStructKeyVec;
        TVector<TSubkey> HashUrlVec;
        TVector<TSubkey> HashOwnerVec;
        TVector<TSubkey> HashUrlMaskVec;
        TVector<TSubkey> HashZDocIdVec;
        TVector<TSubkey> HashQueryDoppelTokenVec;
        TVector<TSubkey> HashQueryDoppelPairVec;
        TVector<TSubkey> HashUrlNoCgiVec;

        THashMap<NQueryDataSaaS::TSaaSKVKey, TString> Sources;

        TVector<TSubkey>* HashVecs[NQueryDataSaaS::SST_IN_KEY_COUNT] {
            nullptr,
            &HashQueryStrongVec,
            &HashQueryDoppelVec,
            &HashUserRegionVec,
            &HashUserIdVec,
            &HashUserLoginVec,
            &HashStructKeyVec,
            &HashUrlVec,
            &HashOwnerVec,
            &HashUrlMaskVec,
            &HashZDocIdVec,
            &HashQueryDoppelTokenVec,
            &HashQueryDoppelPairVec,
            &HashUrlNoCgiVec
        };

        TAllSubkeys() {
            for (int i = NQueryDataSaaS::SST_INVALID + 1; i < NQueryDataSaaS::SST_IN_KEY_COUNT; ++i) {
                UNIT_ASSERT_C(HashVecs[i], i);
            }
        }

        const auto& Get(NQueryDataSaaS::ESaaSSubkeyType st) const {
            return *HashVecs[st];
        }

        auto Get(const TCounter& cnt, const NQueryDataSaaS::TSaaSKeyType& kt) const {
            UNIT_ASSERT(cnt.IsValid());
            UNIT_ASSERT_VALUES_EQUAL(cnt.Size(), kt.size());
            NQueryDataSaaS::TSaaSKVKey key;
            const auto& currentCount = cnt.GetCurrent();
            for (size_t pos = 0; pos < currentCount.size(); ++pos) {
                key.Append((*HashVecs[kt[pos]])[currentCount[pos]]);
            }
            return key;
        }

        auto GetSrc(const NQueryDataSaaS::TSaaSKVKey sk) const {
            UNIT_ASSERT(Sources.contains(sk));
            return Sources.at(sk);
        }

        void Add(NQueryDataSaaS::ESaaSSubkeyType st, const TString& normSource, const TString& rawSource = TString()) {
            auto subkey = NQueryDataSaaS::TSaaSKVKey::From(st, normSource);
            Sources[subkey] = rawSource ? rawSource : normSource;
            HashVecs[st]->emplace_back(subkey);
        }

        void SortAll() {
            for (auto* vec : {&HashUserRegionVec, &HashUrlVec, &HashStructKeyVec}) {
                Sort(*vec);
            }
            for (auto* vec : {&HashOwnerVec, &HashUrlMaskVec, &HashZDocIdVec}) {
                SortUnique(*vec);
            }
        }

        TCounter GetCounter(const NQueryDataSaaS::TSaaSKeyType& kt) const {
            TVector<ui32> sizes;
            for (auto st : kt) {
                sizes.emplace_back(Get(st).size());
            }
            return TCounter(sizes);
        }
    };

    void DoTestKeyGeneration(NQueryDataSaaS::TSaaSRequestRec& rec, const NQueryDataSaaS::TSaaSKeyType& keyType, const TAllSubkeys& allSubkeys) {
        using namespace NQueryDataSaaS;
        UNIT_ASSERT(KeyTypeIsValid(keyType));

        ui32 expectedSize = 1;
        for (auto subkeyType : keyType) {
            expectedSize *= allSubkeys.Get(subkeyType).size();
        }

        TVector<TSaaSKVKey> expectedKeys;
        for (auto cnt = allSubkeys.GetCounter(keyType); cnt.IsValid(); cnt.Next()) {
            expectedKeys.emplace_back(allSubkeys.Get(cnt, keyType));
        }
        SortUnique(expectedKeys);

        UNIT_ASSERT_VALUES_EQUAL_C(expectedSize, expectedKeys.size(), keyType);

        TVector<TSaaSKVKey> keys;
        rec.GenerateKVKeys(keys, keyType);
        UNIT_ASSERT_VALUES_EQUAL_C(keys.size(), expectedSize, keyType);
        SortUnique(keys);
        UNIT_ASSERT_VALUES_EQUAL_C(keys, expectedKeys, keyType);
    }

    void TestKeyGeneration() {
        using namespace NQueryDataSaaS;

        const TString queryStrong{"tok3 tok1 tok2"}, queryDoppel{"tok1    tok2 tok3"};
        const TVector<TString> queryDoppelToken{"tok1", "tok2", "tok3"};
        const TVector<TString> queryDoppelPair{"tok1 tok2", "tok1 tok3", "tok1", "tok2 tok3", "tok2", "tok3"};
        const TString userId{"y1234567890"}, userLogin{"af23ea98"};
        const TVector<TString> regionVec{"213", "1", "3", "225", "10001", "10000"};
        const TVector<TString> urlVec{"aAa", "Aaa/bbb", "aaA/bbb/ccc#ddd", "xxx/yyy?z=123", "zzz"};
        const TVector<std::pair<TString, TString>> structKeyVec{{"ns1", "key1"},
                                                              {"ns1", "key2"},
                                                              {"ns2", "key3"}};
        const TVector<std::pair<TString, TVector<TString>>> structKeyHier{{"ns3", {"key1", "key2", "key3"}}};

        TSaaSRequestRec rec;

        {
            TAllSubkeys subkeys;
            subkeys.Add(SST_IN_KEY_USER_REGION, "0");

            TVector<TSaaSKVKey> keys;
            UNIT_ASSERT_EXCEPTION(rec.GenerateKVKeys(keys, {}), yexception);

            for (int i = SST_INVALID + 1; i < SST_IN_KEY_COUNT; ++i) {
                DoTestKeyGeneration(rec, {ESaaSSubkeyType(i)}, subkeys);

                for (int j = SST_INVALID + 1; j < SST_IN_KEY_COUNT; ++j) {
                    if (i == j) {
                        keys.clear();
                        UNIT_ASSERT_EXCEPTION(rec.GenerateKVKeys(keys, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j}), yexception);
                    } else {
                        DoTestKeyGeneration(rec, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j}, subkeys);

                        for (int k = SST_INVALID + 1; k < SST_IN_KEY_COUNT; ++k) {
                            if (k == i || k == j) {
                                keys.clear();
                                UNIT_ASSERT_EXCEPTION(rec.GenerateKVKeys(keys, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j, (ESaaSSubkeyType) k}), yexception);
                            } else {
                                DoTestKeyGeneration(rec, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j, (ESaaSSubkeyType) k}, subkeys);
                            }
                        }
                    }
                }
            }
        }

        {
            TAllSubkeys subkeys;
            TVector<TSaaSKVKey> keys;

            subkeys.Add(SST_IN_KEY_QUERY_STRONG, queryStrong);
            rec.SetQueryStrong(queryStrong);

            subkeys.Add(SST_IN_KEY_QUERY_DOPPEL, queryDoppel);
            for (auto tokStr : queryDoppelToken) {
                subkeys.Add(SST_IN_KEY_QUERY_DOPPEL_TOKEN, tokStr);
            }
            for (auto pairStr : queryDoppelPair) {
                subkeys.Add(SST_IN_KEY_QUERY_DOPPEL_PAIR, pairStr);
            }
            rec.SetQueryDoppel(queryDoppel);

            subkeys.Add(SST_IN_KEY_USER_ID, userId);
            rec.SetUserId(userId);

            subkeys.Add(SST_IN_KEY_USER_LOGIN_HASH, userLogin);
            rec.SetUserLoginHash(userLogin);

            for (const auto& sk : structKeyVec) {
                subkeys.Add(SST_IN_KEY_STRUCT_KEY, GetStructKeyFromPair(sk.first, sk.second));
                rec.AddStructKey(sk.first, sk.second);
            }

            for (const auto& sk : structKeyHier) {
                for (const auto& k : sk.second) {
                    subkeys.Add(SST_IN_KEY_STRUCT_KEY, GetStructKeyFromPair(sk.first, k));
                }
                rec.AddStructKeyHierarchy(sk.first, sk.second);
            }

            for (const auto& region : regionVec) {
                subkeys.Add(SST_IN_KEY_USER_REGION, region);
            }
            subkeys.Add(SST_IN_KEY_USER_REGION, "0");

            rec.SetUserRegionHierarchy(regionVec);

            for (const auto& url : urlVec) {
                const auto normUrl = NormalizeDocUrl(url);
                const auto owner = TString{GetOwnerFromNormalizedDocUrl(normUrl)};

                subkeys.Add(SST_IN_KEY_URL, normUrl, url);
                subkeys.Add(SST_IN_KEY_OWNER, owner);
                subkeys.Add(SST_IN_KEY_ZDOCID, Url2DocIdSimple(normUrl), Url2DocIdSimple(url));
                subkeys.Add(SST_IN_KEY_URL_NO_CGI, GetNoCgiFromNormalizedUrlStr(normUrl));

                TVector<TString> masks;
                GetMasksFromNormalizedDocUrl(masks, normUrl);
                for (const auto& mask : masks) {
                    subkeys.Add(SST_IN_KEY_URL_MASK, mask);
                }

                rec.AddUrl(url);
            }

            subkeys.SortAll();

            for (int i = SST_INVALID + 1; i < SST_IN_KEY_COUNT; ++i) {
                DoTestKeyGeneration(rec, {ESaaSSubkeyType(i)}, subkeys);

                for (int j = SST_INVALID + 1; j < SST_IN_KEY_COUNT; ++j) {
                    if (i == j) {
                        keys.clear();
                        UNIT_ASSERT_EXCEPTION(rec.GenerateKVKeys(keys, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j}), yexception);
                    } else {
                        DoTestKeyGeneration(rec, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j}, subkeys);

                        for (int k = SST_INVALID + 1; k < SST_IN_KEY_COUNT; ++k) {
                            if (k == i || k == j) {
                                keys.clear();
                                UNIT_ASSERT_EXCEPTION(rec.GenerateKVKeys(keys, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j, (ESaaSSubkeyType) k}), yexception);
                            } else {
                                DoTestKeyGeneration(rec, {(ESaaSSubkeyType) i, (ESaaSSubkeyType) j, (ESaaSSubkeyType) k}, subkeys);
                            }
                        }
                    }
                }
            }
        }
    }

    const TVector<std::pair<TString, TString>> SchemePrefixes{{"", "http://"}, {"http://", "http://"}, {"https://", "https://"}};
    const TVector<TString> HostCases{"ya.ru", "Ya.ru", "ya.rU"};
    const TVector<TString> HostPorts{"", ":80"};
    const TVector<TString> UrlPaths{"", "/", "/Search", "?", "?Search"};
    const TVector<TString> UrlExamples{"https://images.roem.ru/search?text=test", "www.Roem.ru/search?text=test"};
    const TVector<TString> ExpectedMasks{
            "roem.com.tr/",
            "roem.com.tr/search",
            "roem.com.tr/search?text=test"
    };

    void TestUrlNormalization() {
        using namespace NQueryDataSaaS;

        UNIT_ASSERT_VALUES_EQUAL(NormalizeDocUrl(""), "");
        UNIT_ASSERT_VALUES_EQUAL(NormalizeDocUrl("A"), "http://a");

        for (const auto& scheme : SchemePrefixes) {
            for (const auto& host : HostCases) {
                for (const auto& port : HostPorts) {
                    for (const auto& suffix : UrlPaths) {
                        TString input = TString::Join(scheme.first, host, port, suffix);
                        TString expected = TString::Join(scheme.second, "ya.ru", port, suffix);
                        UNIT_ASSERT_VALUES_EQUAL(NormalizeDocUrl(input), expected);
                        UNIT_ASSERT_VALUES_EQUAL(GetZDocIdFromNormalizedUrl(NormalizeDocUrl(input)), Url2DocIdSimple(expected));
                    }
                }
            }
        }

        for (const auto& url : UrlExamples) {
            UNIT_ASSERT_VALUES_EQUAL(GetOwnerFromNormalizedDocUrl(NormalizeDocUrl(url)), "roem.ru");
        }

        TVector<TString> masks;
        GetMasksFromNormalizedDocUrl(masks, "https://roem.com.tr/search?text=test");
        Sort(masks);
        UNIT_ASSERT_VALUES_EQUAL(masks, ExpectedMasks);
    }

    void TestRequestRecUrls() {
        using namespace NQueryDataSaaS;
        const TString urlA = "http://www.roem.ru/Search?text=test";
        const TString inputUrlA = "www.Roem.ru/Search?text=test";
        const TString urlB = "https://images.roem.ru/Search?text=test";
        const TString inputUrlB = "https://images.roem.ru/Search?text=test";
        const TVector<std::pair<TString, TString>> expectedUrls{{urlA, inputUrlA}, {urlB, inputUrlB}};

        const TVector<TStringBuf> inputUrls{
            inputUrlB, inputUrlA
        };

        const TVector<TString> expectedOwners{
            "roem.ru"
        };

        const TVector<std::pair<TStringBuf, TVector<TStringBuf>>> expectedUrlMasks{
            {"images.roem.ru/", {inputUrlB}},
            {"images.roem.ru/Search", {inputUrlB}},
            {"images.roem.ru/Search?text=test", {inputUrlB}},
            {"roem.ru/", inputUrls},
            {"roem.ru/Search", inputUrls},
            {"roem.ru/Search?text=test", inputUrls},
            {"www.roem.ru/", {inputUrlA}},
            {"www.roem.ru/Search", {inputUrlA}},
            {"www.roem.ru/Search?text=test", {inputUrlA}},
        };

        TSaaSRequestRec rec;

        rec.AddUrl("");

        for (const auto& url : inputUrls) {
            rec.AddUrl(url);
        }

        TVector<std::pair<TString, TString>> urls(rec.GetUrlsVec().begin(), rec.GetUrlsVec().end());
        Sort(urls);
        UNIT_ASSERT_VALUES_EQUAL(urls, expectedUrls);

        UNIT_ASSERT(rec.GetOwnersVec().empty());
        rec.GenerateKVSubkeys(SST_IN_KEY_OWNER);
        TVector<TString> owners(rec.GetOwnersVec().begin(), rec.GetOwnersVec().end());
        Sort(owners);
        UNIT_ASSERT_VALUES_EQUAL(owners, expectedOwners);

        rec.GenerateKVSubkeys(SST_IN_KEY_URL_MASK);
        TVector<TStringBuf> urlMaskKeys = rec.GetUrlMasksVec();
        TVector<std::pair<TStringBuf, TVector<TStringBuf>>> urlMasks;
        Sort(urlMaskKeys);
        for (auto& urlMask : urlMaskKeys) {
            urlMasks.emplace_back(urlMask, rec.GetRequestProperties().GetUrlMasksMap().at(urlMask));
            Sort(urlMasks.back().second);
        }
        UNIT_ASSERT_VALUES_EQUAL(urlMasks, expectedUrlMasks);
    }

    TVector<TString> DoConcat(const TVector<TString>& a, const TVector<TString>& b) {
        TVector<TString> res;
        for (const auto& e : a) {
            if (e) {
                res.emplace_back(e);
            }
        }
        res.insert(res.end(), b.begin(), b.end());
        return res;
    }

    using TStructKeysVec = TVector<std::pair<TString, TString>>;
    using TStructKeysHierVec = TVector<std::pair<TString, TVector<TString>>>;

    void DoTestRequestRecFromQD(NQueryDataSaaS::TSaaSRequestRec& rec, const NQueryData::TRequestRec& recQD,
                                bool mobileOp, bool mobileSerp,
                                const TVector<std::pair<TString, TString>>& expectedUrls, const TVector<TString>& expectedOwners,
                                const TStructKeysVec& expectedStructKeys, const TStructKeysHierVec& expectedStructKeysHier
    ) {
        rec.FillFromQueryData(recQD);
        rec.FillFromQueryData(recQD);

        UNIT_ASSERT_VALUES_EQUAL(rec.GetQueryStrong(), recQD.StrongNorm);
        UNIT_ASSERT_VALUES_EQUAL(rec.GetQueryDoppel(), recQD.DoppelNorm);
        UNIT_ASSERT_VALUES_EQUAL(rec.GetUserId(), recQD.UserId);
        UNIT_ASSERT_VALUES_EQUAL(rec.GetUserLoginHash(), recQD.UserLoginHash);
        UNIT_ASSERT_VALUES_EQUAL(rec.GetUserRegionHierarchy(), DoConcat(recQD.UserRegions, {"0"}));
        UNIT_ASSERT_VALUES_EQUAL(rec.GetUserIpRegionHierarchy(), DoConcat(recQD.GetUserRegionsIpReg(), {"0"}));

        if (mobileOp) {
            UNIT_ASSERT_VALUES_EQUAL(rec.GetUserIpOperatorTypeHierarchy(), (TVector<TString>{"mobile_any", "*"}));
        } else {
            UNIT_ASSERT_VALUES_EQUAL(rec.GetUserIpOperatorTypeHierarchy(), (TVector<TString>{"*"}));
        }

        UNIT_ASSERT_VALUES_EQUAL(rec.GetSerpTopLevelDomainHierarchy(), DoConcat({recQD.YandexTLD}, {"*"}));

        if (mobileSerp) {
            UNIT_ASSERT_VALUES_EQUAL(rec.GetSerpDeviceTypeHierarchy(), DoConcat({recQD.SerpType}, {"mobile", "*"}));
        } else {
            UNIT_ASSERT_VALUES_EQUAL(rec.GetSerpDeviceTypeHierarchy(), DoConcat({recQD.SerpType}, {"*"}));
        }

        UNIT_ASSERT_VALUES_EQUAL(rec.GetSerpUILangHierarchy(), DoConcat({recQD.UILang}, {"*"}));


        TStructKeysVec keys{rec.GetStructKeys().begin(), rec.GetStructKeys().end()};
        Sort(keys);
        UNIT_ASSERT_VALUES_EQUAL(keys, expectedStructKeys);

        TStructKeysHierVec hierKeys{rec.GetStructKeyHierarchy().begin(), rec.GetStructKeyHierarchy().end()};
        Sort(hierKeys);
        UNIT_ASSERT_VALUES_EQUAL(hierKeys, expectedStructKeysHier);

        TVector<std::pair<TString, TString>> urls(rec.GetUrlsVec().begin(), rec.GetUrlsVec().end());
        Sort(urls);
        UNIT_ASSERT_VALUES_EQUAL(urls, expectedUrls);
        rec.GenerateKVSubkeys(NQueryDataSaaS::SST_IN_KEY_OWNER);
        TVector<TString> owners(rec.GetOwnersVec().begin(), rec.GetOwnersVec().end());
        Sort(owners);
        UNIT_ASSERT_VALUES_EQUAL(owners, expectedOwners);
    }

    void TestRequestRecFromQD() {
        using namespace NQueryDataSaaS;

        TSaaSRequestRec rec;
        NQueryData::TRequestRec recQD;

        DoTestRequestRecFromQD(rec, recQD, false, false, {}, {}, {}, {});

        recQD.DoppelNorm = "query doppel";
        recQD.StrongNorm = "query strong";
        recQD.UserId = "y123456";
        recQD.UserLogin = "lh_abcdef";
        recQD.UserRegions = {"213", "223", "7", "10000"};
        recQD.UserRegionsIpReg = {"41", "1", "10000"};
        recQD.UserIpMobileOp = "MTS";
        recQD.SerpType = "smart";
        recQD.YandexTLD = "ru";
        recQD.UILang = "en";

        DoTestRequestRecFromQD(rec, recQD, true, true, {}, {}, {}, {});

        recQD.UserIpMobileOpDebug = "0";
        recQD.UserRegionsIpRegDebug = {"42", "11", "10000"};

        DoTestRequestRecFromQD(rec, recQD, false, true, {}, {}, {}, {});

        recQD.DocItems.AddSnipUrl("www.roem.ru/", "yanndex.ru");
        recQD.DocItems.AddUrl("https://Images.Roem.ru/search?text=test", "roem.ru");

        DoTestRequestRecFromQD(rec, recQD, false, true,
                               {
                                   {"http://www.roem.ru/", "www.roem.ru/"},
                                   {"https://images.roem.ru/search?text=test", "https://Images.Roem.ru/search?text=test"}
                               }, {"roem.ru"}, {}, {});

        recQD.StructKeys = NSc::TValue::FromJsonThrow(R"=({
            foo:{
                a:null,b:null,c:null
            },
            bar:[x,y,z],
            '':{f:null},
            f:{'':null},
            baz:['']
        })=");

        DoTestRequestRecFromQD(rec, recQD, false, true,
                               {
                                   {"http://www.roem.ru/", "www.roem.ru/"},
                                   {"https://images.roem.ru/search?text=test", "https://Images.Roem.ru/search?text=test"}
                               }, {"roem.ru"},
                               {{"foo", "a"}, {"foo", "b"}, {"foo", "c"}}, {{"bar", {"x", "y", "z"}}, {"baz", {}}});

    }

    struct TKeyDescr {
        TKeyDescr() = default;

        TKeyDescr(const TString& expectedQd, TStringBuf keyQD, NQueryData::EKeyType keyQDType, const TString& ns = "ns")
            : ExpectedQD(expectedQd)
            , KeyQD(TString{keyQD})
            , KeyQDType(keyQDType)
            , NSpace(ns)
        {}

        TString ExpectedQD;
        TString KeyQD;
        NQueryData::EKeyType KeyQDType = NQueryData::KT_COUNT;
        TString NSpace;
    };

    struct TFilterDescr {
        TFilterDescr() = default;

        TFilterDescr(const TString& expectedQd, NQueryData::EKeyType kt, const TString& key, const TString& ns = "ns", ui32 rejectedByMerge = 0)
            : ExpectedQD(expectedQd)
            , KeyType(kt)
            , Key(key)
            , NSpace(ns)
            , RejectedByMerge(rejectedByMerge)
        {}

        TString ExpectedQD;
        NQueryData::EKeyType KeyType = NQueryData::KT_COUNT;
        TString Key;
        TString NSpace;
        ui32 RejectedByMerge = 0;
    };

    void TestResponseToQD() {
        using namespace NQueryDataSaaS;
        using namespace NQueryData;

        const TString queryStrong = "qStrong";
        const TString queryDoppel = "qDoppel";
        const TString userId = "uId";
        const TString userLogin = "uLogin";
        const TString serpTld = "ru";
        const TString serpDevice = "desktop";
        const TString serpUil = "en";

        const TVector<TString> regionHier{"3", "2", "1"};
        const TVector<TString> regionIpHier{"33", "22", "11"};
        const TVector<std::pair<TString, TString>> structKeys{{"ns1", "key1"}};
        const TVector<std::pair<TString, TVector<TString>>> structKeysHier{{"ns2", {"key1", "key2"}}, {"ns3", {"key3"}}};
        const TVector<TString> urls{"roem.ru/search?lr=213&text=test", "https://google.ru/search?hl=ru&q=test", "https://Images.Roem.ru/search?lr=213&text=test"};
        const TVector<TString> docIds{"Z98FA3DEC95E24C2D", "ZC2253F7A6A0AFCC6"};
        const TVector<TString> urlMasks{"roem.ru/search", "google.ru/search"};
        const TVector<TString> owners{"roem.ru", "google.ru"};

        TSaaSRequestRec rec;
        rec.SetQueryStrong(queryStrong);
        rec.SetQueryDoppel(queryDoppel);
        rec.SetUserId(userId);
        rec.SetUserLoginHash(userLogin);
        rec.SetUserRegionHierarchy(regionHier);
        rec.SetUserIpRegionHierarchy(regionIpHier);
        rec.SetUserIpOperatorType("MTS");
        rec.SetSerpTopLevelDomain(serpTld);
        rec.SetSerpDeviceType(serpDevice);
        rec.SetSerpUILang(serpUil);

        for (const auto& docId : docIds) {
            rec.AddZDocId(docId);
        }
        for (const auto& sk : structKeys) {
            rec.AddStructKey(sk.first, sk.second);
        }
        for (const auto& sk : structKeysHier) {
            rec.AddStructKeyHierarchy(sk.first, sk.second);
        }
        for (const auto& url : urls) {
            rec.AddUrl(url);
        }

        const TVector<TKeyDescr> responseKeyDescrs{
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                queryStrong, KT_QUERY_STRONG
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"qDoppel\" SourceKeyType: KT_QUERY_DOPPEL Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"qDoppel\" Type: KT_QUERY_DOPPEL } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                queryDoppel, KT_QUERY_DOPPEL
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"uId\" SourceKeyType: KT_USER_ID Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"uId\" Type: KT_USER_ID } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                userId, KT_USER_ID
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"uLogin\" SourceKeyType: KT_USER_LOGIN_HASH Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"uLogin\" Type: KT_USER_LOGIN_HASH } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                userLogin, KT_USER_LOGIN_HASH
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"2\" SourceKeyType: KT_USER_REGION Json: \"101\" MergeTraits { Priority: 9223372036854775806 } SourceKeyTraits { IsPrioritized: true } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"2\" Type: KT_USER_REGION Traits { IsPrioritized: true } } Json: \"102\" MergeTraits { Priority: 9223372036854775806 } }",
                regionHier[1], KT_USER_REGION
            },
            {
                "SourceFactors { SourceName: \"ns1\" SourceKey: \"key1\" SourceKeyType: KT_STRUCTKEY Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns1\" SourceSubkeys { Key: \"key1\" Type: KT_STRUCTKEY } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                structKeys[0].second, KT_STRUCTKEY, structKeys[0].first
            },
            {
                "SourceFactors { SourceName: \"ns2\" SourceKey: \"key2\" SourceKeyType: KT_STRUCTKEY Json: \"101\" MergeTraits { Priority: 9223372036854775806 } SourceKeyTraits { IsPrioritized: true } }"
                    " SourceFactors { SourceName: \"ns2\" SourceSubkeys { Key: \"key2\" Type: KT_STRUCTKEY Traits { IsPrioritized: true } } Json: \"102\" MergeTraits { Priority: 9223372036854775806 } }",
                structKeysHier[0].second[1], KT_STRUCTKEY, structKeysHier[0].first
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"Z98FA3DEC95E24C2D\" SourceKeyType: KT_DOCID Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"Z98FA3DEC95E24C2D\" Type: KT_DOCID } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                docIds[0], KT_DOCID
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"ZC2253F7A6A0AFCC6\" SourceKeyType: KT_SNIPDOCID Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"ZC2253F7A6A0AFCC6\" Type: KT_SNIPDOCID } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                docIds[1], KT_SNIPDOCID
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"Z98FA3DEC95E24C2D\" SourceKeyType: KT_DOCID Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"Z98FA3DEC95E24C2D\" Type: KT_DOCID } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                docIds[0], KT_DOCID
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"ZC2253F7A6A0AFCC6\" SourceKeyType: KT_SNIPDOCID Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"ZC2253F7A6A0AFCC6\" Type: KT_SNIPDOCID } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                docIds[1], KT_SNIPDOCID
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"https://Images.Roem.ru/search?lr=213&text=test\" SourceKeyType: KT_URL Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"https://Images.Roem.ru/search?lr=213&text=test\" Type: KT_URL } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                NormalizeDocUrl(urls[2]), KT_URL
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"roem.ru\" SourceKeyType: KT_CATEG Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                owners[0], KT_CATEG
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"google.ru\" SourceKeyType: KT_SNIPCATEG Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"google.ru\" Type: KT_SNIPCATEG } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                owners[1], KT_SNIPCATEG
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"roem.ru/search?lr=213&text=test\" SourceKeyType: KT_URL Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceKey: \"https://Images.Roem.ru/search?lr=213&text=test\" SourceKeyType: KT_URL Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"roem.ru/search?lr=213&text=test\" Type: KT_URL } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"https://Images.Roem.ru/search?lr=213&text=test\" Type: KT_URL } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                urlMasks[0], KT_CATEG_URL,
            },
            {
                "SourceFactors { SourceName: \"ns\" SourceKey: \"roem.ru/search?lr=213&text=test\" SourceKeyType: KT_URL Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceKey: \"https://Images.Roem.ru/search?lr=213&text=test\" SourceKeyType: KT_URL Json: \"101\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"roem.ru/search?lr=213&text=test\" Type: KT_URL } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }"
                    " SourceFactors { SourceName: \"ns\" SourceSubkeys { Key: \"https://Images.Roem.ru/search?lr=213&text=test\" Type: KT_URL } Json: \"102\" MergeTraits { Priority: 9223372036854775807 } }",
                urlMasks[0], KT_SNIPCATEG_URL
            }
        };

        for (const auto& key : responseKeyDescrs) {
            rec.GenerateKVSubkeys(GetSaaSKeyType(key.KeyQDType));

            TSaaSDocument doc;
            TSourceFactors sf1, sf2;
            sf1.SetSourceName(key.NSpace);
            sf2.SetSourceName(key.NSpace);

            sf1.SetJson("101");
            sf2.SetJson("102");

            sf1.SetSourceKeyType(key.KeyQDType);
            sf1.SetSourceKey(key.KeyQD);
            auto* sk = sf2.AddSourceSubkeys();
            sk->SetType(key.KeyQDType);
            sk->SetKey(key.KeyQD);

            doc.Records = {
                Base64Encode(sf1.SerializeAsString()),
                Base64Encode(sf2.SerializeAsString())
            };

            doc.Key = key.KeyQD;

            TQueryData qd;
            auto stats = ProcessSaaSResponse(qd, doc, rec.GetRequestProperties());

            UNIT_ASSERT_VALUES_EQUAL(qd.ShortUtf8DebugString(), key.ExpectedQD);

            if (GetSaaSKeyType(key.KeyQDType) == SST_IN_KEY_URL_MASK) {
                UNIT_ASSERT_VALUES_EQUAL(stats.RecordsAddedByFinalization, 2u);
            } else {
                UNIT_ASSERT_VALUES_EQUAL(stats.RecordsAddedByFinalization, 0);
            }
            UNIT_ASSERT_VALUES_EQUAL(stats.RecordsRejectedByMerge, 0);
            UNIT_ASSERT_VALUES_EQUAL_C(stats.RecordsRejectedAsMalformed, 0, JoinSeq("; ", stats.Errors));
            UNIT_ASSERT_VALUES_EQUAL(stats.RecordsRejectedByRequest, 0);
        }

        const TVector<TFilterDescr> responseFilterDescrs{
            {"", KT_USER_REGION_IPREG, "44"},
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"33\" Type: KT_USER_REGION_IPREG Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"33\" SourceKeyType: KT_USER_REGION_IPREG"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775807 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"33\" Type: KT_USER_REGION_IPREG Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }",
                 KT_USER_REGION_IPREG, "33", "ns", 1
            },
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"0\" Type: KT_USER_REGION_IPREG Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775804 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"0\" SourceKeyType: KT_USER_REGION_IPREG"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775804 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"0\" Type: KT_USER_REGION_IPREG Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775804 } }",
                 KT_USER_REGION_IPREG, "0", "ns", 1
            },
            {"", KT_USER_IP_TYPE, "XXX"},
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"mobile_any\" Type: KT_USER_IP_TYPE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"mobile_any\" SourceKeyType: KT_USER_IP_TYPE"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775807 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"mobile_any\" Type: KT_USER_IP_TYPE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }",
                 KT_USER_IP_TYPE, "mobile_any", "ns", 1
            },
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"*\" Type: KT_USER_IP_TYPE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"*\" SourceKeyType: KT_USER_IP_TYPE"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775806 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"*\" Type: KT_USER_IP_TYPE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }",
                 KT_USER_IP_TYPE, "*", "ns", 1
            },
            {"", KT_SERP_TLD, "com"},
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"ru\" Type: KT_SERP_TLD Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"ru\" SourceKeyType: KT_SERP_TLD"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775807 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"ru\" Type: KT_SERP_TLD Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }",
                 KT_SERP_TLD, "ru", "ns", 1
            },
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"*\" Type: KT_SERP_TLD Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"*\" SourceKeyType: KT_SERP_TLD"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775806 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"*\" Type: KT_SERP_TLD Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }",
                 KT_SERP_TLD, "*", "ns", 1
            },
            {"", KT_SERP_UIL, "ru"},
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"en\" Type: KT_SERP_UIL Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"en\" SourceKeyType: KT_SERP_UIL"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775807 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"en\" Type: KT_SERP_UIL Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }",
                 KT_SERP_UIL, "en", "ns", 1
            },
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"*\" Type: KT_SERP_UIL Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"*\" SourceKeyType: KT_SERP_UIL"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775806 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"*\" Type: KT_SERP_UIL Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }",
                 KT_SERP_UIL, "*", "ns", 1
            },
            {"", KT_SERP_DEVICE, "smart"},
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"desktop\" Type: KT_SERP_DEVICE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"desktop\" SourceKeyType: KT_SERP_DEVICE"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775807 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"desktop\" Type: KT_SERP_DEVICE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775807 } }",
                 KT_SERP_DEVICE, "desktop", "ns", 1
            },
            {
             "SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"*\" Type: KT_SERP_DEVICE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }"
//                 " SourceFactors { SourceName: \"ns\" SourceKey: \"*\" SourceKeyType: KT_SERP_DEVICE"
//                 " SourceSubkeys { Key: \"qStrong\" Type: KT_QUERY_STRONG } MergeTraits { Priority: 9223372036854775806 } SourceKeyTraits { IsPrioritized: true } }"
                 " SourceFactors { SourceName: \"ns\" SourceKey: \"qStrong\" SourceKeyType: KT_QUERY_STRONG"
                 " SourceSubkeys { Key: \"roem.ru\" Type: KT_CATEG }"
                 " SourceSubkeys { Key: \"*\" Type: KT_SERP_DEVICE Traits { IsPrioritized: true } } MergeTraits { Priority: 9223372036854775806 } }",
                 KT_SERP_DEVICE, "*", "ns", 1
            },
        };

        for (const auto& key : responseFilterDescrs) {
            TSaaSDocument doc1, doc2;
            TSourceFactors sf1, sf2, sf3;
            sf1.SetSourceName(key.NSpace);
            sf1.SetSourceKeyType(KT_QUERY_STRONG);
            sf1.SetSourceKey(queryStrong);
            {
                auto* sk = sf1.AddSourceSubkeys();
                sk->SetKey(key.Key);
                sk->SetType(key.KeyType);
            }

            sf2.SetSourceName(key.NSpace);
            sf2.SetSourceKey(key.Key);
            sf2.SetSourceKeyType(key.KeyType);
            {
                auto* sk = sf2.AddSourceSubkeys();
                sk->SetType(KT_QUERY_STRONG);
                sk->SetKey(queryStrong);
            }

            doc1.Key = queryStrong;
            doc1.Records = {
                Base64Encode(sf1.SerializeAsString()),
                Base64Encode(sf2.SerializeAsString())
            };

            sf3.SetSourceName(key.NSpace);
            sf3.SetSourceKeyType(KT_QUERY_STRONG);
            sf3.SetSourceKey(queryStrong);
            {
                auto* sk = sf3.AddSourceSubkeys();
                sk->SetType(KT_CATEG);
                sk->SetKey(owners[0]);
            }
            {
                auto* sk = sf3.AddSourceSubkeys();
                sk->SetKey(key.Key);
                sk->SetType(key.KeyType);
            }

            doc2.Key = queryStrong + owners[0];
            doc2.Records = {
                Base64Encode(sf3.SerializeAsString())
            };

            TQueryData qd;
            auto stats = ProcessSaaSResponse(qd, doc1, rec.GetRequestProperties());
            stats += ProcessSaaSResponse(qd, doc2, rec.GetRequestProperties());

            UNIT_ASSERT_VALUES_EQUAL(qd.ShortUtf8DebugString(), key.ExpectedQD);

            UNIT_ASSERT_VALUES_EQUAL_C(stats.RecordsAddedByFinalization, 0, key.ExpectedQD);
            UNIT_ASSERT_VALUES_EQUAL_C(stats.RecordsRejectedByMerge, key.RejectedByMerge, key.ExpectedQD);
            UNIT_ASSERT_VALUES_EQUAL_C(stats.RecordsRejectedAsMalformed, 0, JoinSeq("; ", stats.Errors));

            if (key.ExpectedQD) {
                UNIT_ASSERT_VALUES_EQUAL_C(stats.RecordsRejectedByRequest, 0, key.ExpectedQD);
            } else {
                UNIT_ASSERT_VALUES_EQUAL(stats.RecordsRejectedByRequest, 3u);
            }
        }
    }
};

UNIT_TEST_SUITE_REGISTRATION(TSaaSQuerySearchConv);

#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <kernel/querydata/scheme/qd_scheme.h>

#include <library/cpp/testing/unittest/registar.h>

class TQDSchemeTest : public TTestBase {
    UNIT_TEST_SUITE(TQDSchemeTest)
        UNIT_TEST(TestQueryData2Scheme)
    UNIT_TEST_SUITE_END();

    void DoTestQueryData2Scheme(TStringBuf querydata, TStringBuf expected_v1, TStringBuf expected_v2) {
        NQueryData::TQueryData qd;
        NQueryData::NTests::BuildQDJson(qd, querydata);
        NSc::NUt::AssertSchemeJson(expected_v1, NQueryData::QueryData2Scheme(qd));
        NSc::NUt::AssertSchemeJson(expected_v2, NQueryData::QueryData2SchemeV2(qd));
    }

    void TestQueryData2Scheme() {
        DoTestQueryData2Scheme("{SourceFactors:[{"
                        "SourceName:src,"
                        "TrieName:src_trie,"
                        "SourceKey:test,"
                        "SourceKeyType:doppel,"
                        "Version:1351186794,"
                        "Factors:[{Name:sf,StringValue:sfv},"
                        "{Name:ff,FloatValue:1.5},"
                        "{Name:if,IntValue:3},"
                        "{Name:bf,BinaryValue:'YmludGVzdA=='}]"
                        "},{"
                        "SourceName:src,"
                        "TrieName:src_trie,"
                        "Common:true,"
                        "Version:1351186794,"
                        "Factors:[{Name:csf,StringValue:csfv},{Name:cff,FloatValue:2.5},{Name:cif,IntValue:5}]"
                        "},{"
                        "SourceName:src,"
                        "TrieName:src_trie,"
                        "SourceKey:test2,"
                        "SourceKeyType:doppeltok,"
                        "Version:1351186794,"
                        "SourceSubkeys:[{Key:test3,Type:yid},{Key:'213',Type:region}],"
                        "Factors:[{Name:tsf,StringValue:tsfv},{Name:tff,FloatValue:3.5},{Name:tif,IntValue:7}]"
                        "},{"
                        "SourceName:src,"
                        "TrieName:src_trie,"
                        "SourceKey:test3,"
                        "SourceKeyType:docid,"
                        "Version:1351186794,"
                        "Json:{a:b,c:d,e:[f,g]}"
                        "},{"
                        "SourceName:src,"
                        "TrieName:src_trie,"
                        "Common:true,"
                        "Version:1351186794,"
                        "Json:{z:y,w:x,v:[u,t]}"
                        "}]}"
                        ,
                        "{src:{timestamp:1351186794,"
                        "common:{csf:csfv,cff:2.5,cif:5,z:y,w:x,v:[u,t]},"
                        "values:{sf:sfv,ff:1.5,if:3,bf:null,test2:{'213':{tsf:tsfv,tff:3.5,tif:7}},test3:{a:b,c:d,e:[f,g]}}"
                        "}}"
                        ,
                        "["
                            "{Namespace:src,Timestamp:1351186794,TrieName:src_trie"
                                ",HostName:''"
                                ",IsRealtime:0"
                                ",ShardNumber:0"
                                ",ShardsTotal:0"
                                ",Key:[test]"
                                ",KeyType:[doppel]"
                                ",Value:{sf:sfv,ff:1.5,if:3,bf:'YmludGVzdA=='}}"
                           ",{Namespace:src,Timestamp:1351186794,TrieName:src_trie"
                                ",HostName:''"
                                ",IsRealtime:0"
                                ",ShardNumber:0"
                                ",ShardsTotal:0"
                                ",IsCommon:1"
                                ",Value:{csf:csfv,cff:2.5,cif:5}}"
                           ",{Namespace:src,Timestamp:1351186794,TrieName:src_trie"
                                ",HostName:''"
                                ",IsRealtime:0"
                                ",ShardNumber:0"
                                ",ShardsTotal:0"
                                ",Key:[test2,test3,'213']"
                                ",KeyType:[doppeltok,yid,region]"
                                ",Value:{tsf:tsfv,tff:3.5,tif:7}}"
                           ",{Namespace:src,Timestamp:1351186794,TrieName:src_trie"
                                ",HostName:''"
                                ",IsRealtime:0"
                                ",ShardNumber:0"
                                ",ShardsTotal:0"
                                ",Key:[test3]"
                                ",KeyType:[docid]"
                                ",Value:{a:b,c:d,e:[f,g]}}"
                           ",{Namespace:src,Timestamp:1351186794,TrieName:src_trie"
                                ",HostName:''"
                                ",IsRealtime:0"
                                ",ShardNumber:0"
                                ",ShardsTotal:0"
                                ",IsCommon:1"
                                ",Value:{z:y,w:x,v:[u,t]}}"
                        "]");
    }
};

UNIT_TEST_SUITE_REGISTRATION(TQDSchemeTest)

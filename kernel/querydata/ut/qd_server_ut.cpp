#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <kernel/querydata/scheme/qd_scheme.h>
#include <kernel/querydata/server/qd_server.h>
#include <kernel/querydata/server/qd_source.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/string/escape.h>

class TQueryDataServerTest : public TTestBase {
    UNIT_TEST_SUITE(TQueryDataServerTest)
        UNIT_TEST(TestRegions);
        UNIT_TEST(TestShards);
    UNIT_TEST_SUITE_END();
private:

    void DoTestQuery(const NQueryData::TQueryDatabase& db, TStringBuf request, TStringBuf qdresult, TStringBuf expresult) {
        using namespace NQueryData;
        TRequestRec rec;
        AdaptQueryDataRequest(rec, NSc::TValue::FromJson(request));
        TQueryData qd;
        db.GetQueryData(rec, qd);
        NSc::NUt::AssertSchemeJson(qdresult, NSc::TValue::From(qd));
        NSc::TValue resval;
        QueryData2Scheme(resval, qd);
        UNIT_ASSERT_JSON_EQ_JSON_C(resval, expresult, request);
    }

    void DoTestStats(const NQueryData::TQueryDatabase& db, NQueryData::EStatsVerbosity sv, TStringBuf refstjson) {
        NSc::TValue stats = db.GetStats(sv);
        UNIT_ASSERT_C(stats["stats"]["memory-usage"].GetIntNumber() > stats["stats"]["total-size"].GetIntNumber()
                      , stats.ToJson(true));
        stats["stats"].Delete("memory-usage");
        UNIT_ASSERT_JSON_EQ_JSON(stats, refstjson);
    }

    void TestRegions() {
        using namespace NQueryData;
        TServerOpts opts;
        opts.EnableDebugInfo = true;
        TQueryDatabase db(opts);

        db.AddSource(NTests::MakeFakeSourceJson("{"
                        " SourceName:test"
                        ",SourceKeys:[KT_STRUCTKEY,KT_SERP_TLD,KT_USER_REGION]"
                        ",FactorNames:{f:1}"
                        ",Version:1400050000"
                        ",IndexingTimestamp:1410050000"
                        ",FileTimestamp:1420050000"
                        ",HasJson:true"
                        ",HasKeyRef:1"
                        ",Shards:2"
                        ",ShardNumber:1"
                        ",SourceData:{"
                            " keyA:{"
                                " '*':{'213':{f:'A*213',Json:{A:'*213'}}"
                                     ",'1':{f:'A*1',Json:{A:'*1'}}"
                                     ",'10001':{f:'A*10001',Json:{A:'*10001'}}}"
                                ",'ru':{'213':{f:'Aru213',Json:{A:ru213}}"
                                "      ,'3':{f:'Aru3',Json:{A:ru3}}"
                                "      ,'10000':{f:'Aru10000',Json:{A:ru10000}}}"
                            "}"
                            ",keyB:{"
                                " '*':{'213':{f:'B*213'},'1':{f:'B*1'},'10001':{f:'B*10001'}}"
                                ",'ru':{'225':{f:'Bru225'},'3':{f:'Bru3'},'10000':{f:'Bru10000'}}"
                            "}"
                            ",keyC:{"
                                " '*':{'213':{f:'C*213'},'1':{f:'C*1'},'10001':{f:'C*10001'}}"
                                ",'ru':{'600':{f:'Cru600'}}"
                            "}"
                            ",keyD:{"
                                " 'com':{'1':'keyA\tru\t213'}}"
                        "}}"));
        db.AddSource(NTests::MakeFakeSourceJson("{"
                        " SourceName:test1"
                        ",SourceKeys:[KT_STRUCTKEY,KT_SERP_TLD,KT_USER_REGION]"
                        ",FactorNames:{f:1}"
                        ",Version:1400050000"
                        ",IndexingTimestamp:1410050000"
                        ",FileTimestamp:1420050000"
                        ",HasJson:true"
                        ",HasKeyRef:1"
                        ",Shards:2"
                        ",ShardNumber:1"
                        ",SourceData:{"
                            " keyA:{"
                                " '*':{'213':{f:'A*213',Json:{A:'*213'}}"
                                     ",'1':{f:'A*1',Json:{A:'*1'}}"
                                     ",'10001':{f:'A*10001',Json:{A:'*10001'}}}"
                                ",'ru':{'213':{f:'Aru213',Json:{A:ru213}}"
                                "      ,'3':{f:'Aru3',Json:{A:ru3}}"
                                "      ,'10000':{f:'Aru10000',Json:{A:ru10000}}}"
                            "}"
                            ",keyB:{"
                                " '*':{'213':{f:'B*213'},'1':{f:'B*1'},'10001':{f:'B*10001'}}"
                                ",'ru':{'225':{f:'Bru225'},'3':{f:'Bru3'},'10000':{f:'Bru10000'}}"
                            "}"
                            ",keyC:{"
                                " '*':{'213':{f:'C*213'},'1':{f:'C*1'},'10001':{f:'C*10001'}}"
                                ",'ru':{'600':{f:'Cru600'}}"
                            "}"
                            ",keyD:{"
                                " 'com':{'1':'keyA\tru\t213'}}"
                        "}}"));
        db.AddSource(NTests::MakeFakeSourceJson("{"
                        " SourceName:test2"
                        ",SourceKeys:[KT_STRUCTKEY,KT_SERP_TLD,KT_USER_REGION]"
                        ",FactorNames:{ff:1}"
                        ",Version:1400010000"
                        ",IndexingTimestamp:1410010000"
                        ",FileTimestamp:1420010000"
                        ",CommonFactors:{xx:yy}"
                        ",SourceData:{"
                            " keyX:{"
                                " '*':{"
                                    "'213':{ff:'X*213',Version:1400020000}"
                                    ",'1':{ff:'X*1',Version:1400020000}"
                                    ",'10001':{ff:'X*10001',Version:1400020000}"
                                "}"
                                ",'ru':{"
                                    "'213':{ff:'Xru213',Version:1400020000}"
                                    ",'3':{ff:'Xru3',Version:1400020000}"
                                    ",'10000':{ff:'Xru10000',Version:1400020000}"
                                "}"
                            "}"
                            ",keyY:{"
                                " '*':{"
                                    "'213':{ff:'Y*213',Version:1400020000}"
                                    ",'1':{ff:'Y*1',Version:1400020000}"
                                    ",'10001':{ff:'Y*10001',Version:1400020000}"
                                "}"
                                ",'ru':{"
                                    "'225':{ff:'Yru225',Version:1400020000}"
                                    ",'3':{ff:'Yru3',Version:1400020000}"
                                    ",'10000':{ff:'Yru10000',Version:1400020000}"
                                "}"
                            "}"
                            ",keyZ:{"
                                " '*':{"
                                    "'213':{ff:'Z*213',Version:1400020000}"
                                    ",'':{ff:'XXX',Version:1400020000}"
                                    ",'1':{ff:'Z*1',Version:1400020000}"
                                    ",'10001':{ff:'Z*10001',Version:1400020000}"
                                "}"
                                ",'':{"
                                    "'213':{ff:'XXX',Version:1400020000}"
                                "}"
                                ",'ru':{'600':{ff:'Zru600',Version:1400020000}}"
                            "}"
                            ",'':{"
                                "'*':{"
                                    "'213':{ff:'XXX',Version:1400020000}"
                                "}"
                            "}"
                        "}}"));

        DoTestQuery(db, "{StructKeys:{test:{keyD:1}},UserRegions:['1'],YandexTLD:com}"
                        , "{SourceFactors:["
                        "{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967295}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyD"
                            ",SourceSubkeys:["
                                " {Key:com,Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'1',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:Aru213,Name:f}]"
                            ",Json:'{\"A\":\"ru213\"}'"
                            ",SourceName:test"
                            ",TrieName:'test.trie.1400050000'"
                        "},{"
                            " Common:true"
                            ",Factors:[{Name:xx,StringValue:yy}]"
                            ",SourceName:test2"
                            ",Version:1400010000"
                            ",TrieName:'test2.trie.1400010000'"
                        "}"
                        "]}"
                        ,"{"
                        " test:{"
                        "  timestamp:1400050000"
                        ", values:{keyD:{'com':{'1':{f:Aru213,A:ru213}}}}"
                        "},"
                        " test2:{"
                        "  common:{xx:yy},"
                        "  timestamp:1400010000"
                        "}"
                        "}");
        DoTestQuery(db, "{"
                            "StructKeys:{"
                                "test:{keyA:1,keyB:1,keyC:1}"
                               ",test1:keyA"
                               ",test2:[keyZ,keyX,keyY]"
                            "}"
                           ",UserRegions:["
                               "'213','1','3','225','10001','10000'"
                            "]"
                           ",YandexTLD:ru"
                        "}"
                        ,
                        "{SourceFactors:["
                        "{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967293}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyB"
                            ",SourceSubkeys:["
                                " {Key:ru,Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'3',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:Bru3,Name:f}]"
                            ",SourceName:test"
                            ",TrieName:'test.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967039}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyC"
                            ",SourceSubkeys:["
                                " {Key:'*',Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'213',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:'C*213',Name:f}]"
                            ",SourceName:test"
                            ",TrieName:'test.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967295}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyA"
                            ",SourceSubkeys:["
                                " {Key:ru,Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'213',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:Aru213,Name:f}]"
                            ",Json:'{\"A\":\"ru213\"}'"
                            ",SourceName:test"
                            ",TrieName:'test.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967295}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyA"
                            ",SourceSubkeys:["
                                " {Key:ru,Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'213',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:Aru213,Name:f}]"
                            ",Json:'{\"A\":\"ru213\"}'"
                            ",SourceName:test1"
                            ",TrieName:'test1.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:true}"
                            ",MergeTraits:{Priority:4294967039}"
                            ",Version:1400020000"
                            ",SourceKey:keyZ"
                            ",SourceSubkeys:["
                                " {Key:'*',Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'213',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:'Z*213',Name:ff}]"
                            ",SourceName:test2"
                            ",TrieName:'test2.trie.1400010000'"
                        "},{"
                            " Common:true"
                            ",Factors:[{Name:xx,StringValue:yy}]"
                            ",SourceName:test2"
                            ",Version:1400010000"
                            ",TrieName:'test2.trie.1400010000'"
                        "}]}"
                        ,"{"
                        "test:{"
                        "  timestamp:1400050000"
                        ", values:{keyA:{'ru':{'213':{f:Aru213,A:ru213}}},keyB:{'ru':{'3':{f:Bru3}}},keyC:{'*':{'213':{f:'C*213'}}}}"
                        "},"
                        "test1:{"
                        "  timestamp:1400050000"
                        ", values:{keyA:{'ru':{'213':{f:Aru213,A:ru213}}}}"
                        "},"
                        "test2:{"
                        "  timestamp:1400020000"
                        ", values:{keyZ:{'*':{'213':{ff:'Z*213'}}}}"
                        ", common:{xx:yy}"
                        "}}");

        DoTestQuery(db, "{"
                            "StructKeys:{"
                                 "test:{keyA:1,keyB:1,keyC:1}"
                                ",test1:keyA"
                                ",test2:[keyZ,keyX,keyY]"
                            "}"
                           ",UserRegions:["
                               "'213','1','3','225','10001','10000'"
                            "]"
                           ",FilterNamespaces:{test1:1,test2:1}"
                           ",SkipNamespaces:{test1:1}"
                           ",YandexTLD:ru"
                        "}"
                        ,
                        "{SourceFactors:["
                        "{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:true}"
                            ",MergeTraits:{Priority:4294967039}"
                            ",Version:1400020000"
                            ",SourceKey:keyZ"
                            ",SourceSubkeys:["
                                " {Key:'*',Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'213',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:'Z*213',Name:ff}]"
                            ",SourceName:test2"
                            ",TrieName:'test2.trie.1400010000'"
                        "},{"
                            " Common:true"
                            ",Factors:[{Name:xx,StringValue:yy}]"
                            ",SourceName:test2"
                            ",Version:1400010000"
                            ",TrieName:'test2.trie.1400010000'"
                        "}]}"
                        ,"{"
                        "test2:{"
                        "  timestamp:1400020000"
                        ", values:{keyZ:{'*':{'213':{ff:'Z*213'}}}}"
                        ", common:{xx:yy}"
                        "}}");

        DoTestStats(db, SV_MAIN, "{"
                        "fakes:{"
                            "fake:["
                                "{"
                                    "file:{"
                                        "'load-mode':ram,"
                                        "size:0,"
                                        "timestamp:1420050000,"
                                        "'timestamp-hr':'2014.12.31 18:20:00 UTC'"
                                    "},"
                                    "source:{"
                                        "'indexing-timestamp':1410050000,"
                                        "'indexing-timestamp-hr':'2014.09.07 00:33:20 UTC',"
                                        "name:test,"
                                        "shard:'1/2',"
                                        "'shard-num':1,"
                                        "'shard-total':2,"
                                        "size:460,"
                                        "'size-ram':460,"
                                        "timestamp:1400050000,"
                                        "'timestamp-hr':'2014.05.14 06:46:40 UTC'"
                                    "}"
                                "},{"
                                    "file:{"
                                        "'load-mode':ram,"
                                        "size:0,"
                                        "timestamp:1420050000,"
                                        "'timestamp-hr':'2014.12.31 18:20:00 UTC'"
                                    "},"
                                    "source:{"
                                        "'indexing-timestamp':1410050000,"
                                        "'indexing-timestamp-hr':'2014.09.07 00:33:20 UTC',"
                                        "name:test1,"
                                        "shard:'1/2',"
                                        "'shard-num':1,"
                                        "'shard-total':2,"
                                        "size:460,"
                                        "'size-ram':460,"
                                        "timestamp:1400050000,"
                                        "'timestamp-hr':'2014.05.14 06:46:40 UTC'"
                                    "}"
                                "},{"
                                    "file:{"
                                        "'load-mode':ram,"
                                        "size:0,"
                                        "timestamp:1420010000,"
                                        "'timestamp-hr':'2014.12.31 07:13:20 UTC'"
                                    "},"
                                    "source:{"
                                        "'indexing-timestamp':1410010000,"
                                        "'indexing-timestamp-hr':'2014.09.06 13:26:40 UTC',"
                                        "name:test2,"
                                        "shard:'0/0',"
                                        "'shard-num':-1,"
                                        "'shard-total':-1,"
                                        "size:513,"
                                        "'size-ram':513,"
                                        "timestamp:1400010000,"
                                        "'timestamp-hr':'2014.05.13 19:40:00 UTC'"
                                    "}"
                                "}"
                            "]"
                        "},"
                        "stats:{"
                            "'enable-debug-info':1,"
                            "'enable-fast-mmap':0,"
                            "'max-memory-usage':-1,"
                            "'max-total-ram-size':11274289152,"
                            "'max-trie-ram-size':6442450944,"
                            "'min-trie-fast-mmap-size':1073741824,"
                            "'total-ram-size':1433,"
                            "'total-size':1433,"
                            "'tries-count-all':3,"
                            "'tries-count-invalid':0,"
                            "'tries-count-fast-mmap':0,"
                            "'tries-count-mmap':0,"
                            "'tries-count-ram':3"
                        "}"
                    "}");
        DoTestStats(db, SV_SOURCES, "{"
                        "fakes:{"
                            "fake:["
                                "{"
                                    "file:{"
                                        "'load-mode':ram,"
                                        "size:0,"
                                        "timestamp:1420050000,"
                                        "'timestamp-hr':'2014.12.31 18:20:00 UTC'"
                                    "},source:{"
                                        "auxnorm:[tld,region],"
                                        "'command-line':[],"
                                        "'indexer-host':'',"
                                        "compression:MOCK,"
                                        "'has-json':1,"
                                        "'has-keyref':1,"
                                        "'indexing-timestamp':1410050000,"
                                        "'indexing-timestamp-hr':'2014.09.07 00:33:20 UTC',"
                                        "name:test,"
                                        "norm:structkey,"
                                        "shard:'1/2',"
                                        "'shard-num':1,"
                                        "'shard-total':2,"
                                        "size:460,"
                                        "'size-ram':460,"
                                        "timestamp:1400050000,"
                                        "'timestamp-hr':'2014.05.14 06:46:40 UTC',"
                                        "trie:comptrie"
                                    "}"
                                "},{"
                                    "file:{"
                                        "'load-mode':ram,"
                                        "size:0,"
                                        "timestamp:1420050000,"
                                        "'timestamp-hr':'2014.12.31 18:20:00 UTC'"
                                    "},source:{"
                                        "auxnorm:[tld,region],"
                                        "'command-line':[],"
                                        "'indexer-host':'',"
                                        "compression:MOCK,"
                                        "'has-json':1,"
                                        "'has-keyref':1,"
                                        "'indexing-timestamp':1410050000,"
                                        "'indexing-timestamp-hr':'2014.09.07 00:33:20 UTC',"
                                        "name:test1,"
                                        "norm:structkey,"
                                        "shard:'1/2',"
                                        "'shard-num':1,"
                                        "'shard-total':2,"
                                        "size:460,"
                                        "'size-ram':460,"
                                        "timestamp:1400050000,"
                                        "'timestamp-hr':'2014.05.14 06:46:40 UTC',"
                                        "trie:comptrie"
                                    "}"
                                "},{"
                                    "file:{"
                                        "'load-mode':ram,"
                                        "size:0,"
                                        "timestamp:1420010000,"
                                        "'timestamp-hr':'2014.12.31 07:13:20 UTC'"
                                    "},source:{"
                                        "auxnorm:[tld,region],"
                                        "'command-line':[],"
                                        "'indexer-host':'',"
                                        "compression:MOCK,"
                                        "'has-json':0,"
                                        "'has-keyref':0,"
                                        "'indexing-timestamp':1410010000,"
                                        "'indexing-timestamp-hr':'2014.09.06 13:26:40 UTC',"
                                        "name:test2,"
                                        "norm:structkey,"
                                        "shard:'0/0',"
                                        "'shard-num':-1,"
                                        "'shard-total':-1,"
                                        "size:513,"
                                        "'size-ram':513,"
                                        "timestamp:1400010000,"
                                        "'timestamp-hr':'2014.05.13 19:40:00 UTC',"
                                        "trie:comptrie"
                                    "}"
                                "}"
                            "]"
                        "},"
                        "stats:{"
                            "'enable-debug-info':1,"
                            "'enable-fast-mmap':0,"
                            "'max-memory-usage':-1,"
                            "'max-total-ram-size':11274289152,"
                            "'max-trie-ram-size':6442450944,"
                            "'min-trie-fast-mmap-size':1073741824,"
                            "'total-ram-size':1433,"
                            "'total-size':1433,"
                            "'tries-count-all':3,"
                            "'tries-count-invalid':0,"
                            "'tries-count-fast-mmap':0,"
                            "'tries-count-mmap':0,"
                            "'tries-count-ram':3"
                        "}"
                    "}");

        for (auto q : {"", "keyZ\t", "\t*", "\t*\t", "keyZ\t*\t213\t", "\tkeyZ\t*\t213"}) {
            TQueryData tmpQd;
            db.GetQueryData(q, tmpQd);
            UNIT_ASSERT_VALUES_EQUAL_C(tmpQd.SourceFactorsSize(), 1u, EscapeC(q) + "\n" + tmpQd.DebugString());
        }
        for (auto q : {"keyZ\t*\t", "\t*\t213", "keyZ\t\t213", "keyZ\t*\t213"}) {
            TQueryData tmpQd;
            db.GetQueryData(q, tmpQd);
            UNIT_ASSERT_VALUES_EQUAL_C(tmpQd.SourceFactorsSize(), 2u, EscapeC(q) + "\n" + tmpQd.DebugString());
        }
    }

    void TestShards() {
        using namespace NQueryData;
        TServerOpts opts;
        opts.EnableDebugInfo = true;
        TQueryDatabase db(opts);

        db.AddSource(NTests::MakeFakeSourceJson("{"
                        " SourceName:test-0"
                        ",SourceKeys:[KT_STRUCTKEY,KT_SERP_TLD,KT_USER_REGION]"
                        ",FactorNames:{f:1}"
                        ",Version:1400050000"
                        ",IndexingTimestamp:1410050000"
                        ",FileTimestamp:1420050000"
                        ",HasJson:true"
                        ",Shards:2"
                        ",ShardNumber:0"
                        ",HasKeyRef:0"
                        ",SourceData:{"
                            " keyA:{"
                                " '*':{'213':{f:'A*213'},'1':{f:'A*1'},'10001':{f:'A*10001'}}"
                                ",'ru':{'213':{f:'Aru213'},'3':{f:'Aru3'},'10000':{f:'Aru10000'}}"
                            "}"
                            ",keyB:{"
                                " '*':{'213':{f:'B*213'},'1':{f:'B*1'},'10001':{f:'B*10001'}}"
                                ",'ru':{'225':{f:'Bru225'},'3':{f:'Bru3'},'10000':{f:'Bru10000'}}"
                            "}"
                            ",keyC:{"
                                " '*':{'213':{f:'C*213'},'1':{f:'C*1'},'10001':{f:'C*10001'}}"
                                ",'ru':{'600':{f:'Cru600'}}"
                            "}"
                        "}}"));

        db.AddSource(NTests::MakeFakeSourceJson("{"
                        " SourceName:test-1"
                        ",SourceKeys:[KT_STRUCTKEY,KT_SERP_TLD,KT_USER_REGION]"
                        ",FactorNames:{f:1}"
                        ",Version:1400050000"
                        ",IndexingTimestamp:1410050000"
                        ",FileTimestamp:1420050000"
                        ",HasJson:true"
                        ",Shards:2"
                        ",ShardNumber:1"
                        ",HasKeyRef:0"
                        ",SourceData:{"
                            " keyA:{"
                                " '*':{'213':{f:'A*213'},'1':{f:'A*1'},'10001':{f:'A*10001'}}"
                                ",'ru':{'213':{f:'Aru213'},'3':{f:'Aru3'},'10000':{f:'Aru10000'}}"
                            "}"
                            ",keyB:{"
                                " '*':{'213':{f:'B*213'},'1':{f:'B*1'},'10001':{f:'B*10001'}}"
                                ",'ru':{'225':{f:'Bru225'},'3':{f:'Bru3'},'10000':{f:'Bru10000'}}"
                            "}"
                            ",keyC:{"
                                " '*':{'213':{f:'C*213'},'1':{f:'C*1'},'10001':{f:'C*10001'}}"
                                ",'ru':{'600':{f:'Cru600'}}"
                            "}"
                        "}}"));

        DoTestQuery(db, "{StructKeys:{test-0:{keyA:1,keyB:1,keyC:1},test-1:{keyA:1,keyB:1,keyC:1}},"
                        "UserRegions:['213','1','3','225','10001','10000'],YandexTLD:ru}"
                        ,"{SourceFactors:["
                        "{"
                            " Factors:[{Name:f,StringValue:Bru10000}]"
                            ",MergeTraits:{Priority:4294967290}"
                            ",ShardNumber:0"
                            ",ShardsTotal:2"
                            ",SourceKey:keyB"
                            ",SourceKeyTraits:{IsPrioritized:false,MustBeInScheme:true}"
                            ",SourceKeyType:KT_STRUCTKEY"
                            ",SourceName:'test-0'"
                            ",SourceSubkeys:["
                                " {Key:ru,Traits:{IsPrioritized:true,MustBeInScheme:true},Type:KT_SERP_TLD}"
                                ",{Key:'10000',Traits:{IsPrioritized:true,MustBeInScheme:true},Type:KT_USER_REGION}]"
                            ",Version:1400050000"
                            ",TrieName:'test-0.trie.1400050000'"
                        "},{"
                            " Factors:[{Name:f,StringValue:'C*213'}]"
                            ",MergeTraits:{Priority:4294967039}"
                            ",ShardNumber:0"
                            ",ShardsTotal:2"
                            ",SourceKey:keyC"
                            ",SourceKeyTraits:{IsPrioritized:false,MustBeInScheme:true}"
                            ",SourceKeyType:KT_STRUCTKEY"
                            ",SourceName:'test-0'"
                            ",SourceSubkeys:["
                                " {Key:'*',Traits:{IsPrioritized:true,MustBeInScheme:true},Type:KT_SERP_TLD}"
                                ",{Key:'213',Traits:{IsPrioritized:true,MustBeInScheme:true},Type:KT_USER_REGION}]"
                            ",Version:1400050000"
                            ",TrieName:'test-0.trie.1400050000'"
                        "},{"
                            " Factors:[{Name:f,StringValue:Aru3}]"
                            ",MergeTraits:{Priority:4294967293}"
                            ",ShardNumber:0"
                            ",ShardsTotal:2"
                            ",SourceKey:keyA"
                            ",SourceKeyTraits:{IsPrioritized:false,MustBeInScheme:true}"
                            ",SourceKeyType:KT_STRUCTKEY"
                            ",SourceName:'test-0'"
                            ",SourceSubkeys:["
                                " {Key:ru,Traits:{IsPrioritized:true,MustBeInScheme:true},Type:KT_SERP_TLD}"
                                ",{Key:'3',Traits:{IsPrioritized:true,MustBeInScheme:true},Type:KT_USER_REGION}]"
                            ",Version:1400050000"
                            ",TrieName:'test-0.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967293}"
                            ",Version:1400050000"
                            ",SourceKey:keyB"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceSubkeys:["
                                " {Key:ru,Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'3',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:Bru3,Name:f}]"
                            ",SourceName:test-1"
                            ",TrieName:'test-1.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967038}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyC"
                            ",SourceSubkeys:["
                                " {Key:'*',Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'1',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:'C*1',Name:f}]"
                            ",SourceName:test-1"
                            ",TrieName:'test-1.trie.1400050000'"
                        "},{"
                            " SourceKeyType:KT_STRUCTKEY"
                            ",SourceKeyTraits:{MustBeInScheme:true,IsPrioritized:false}"
                            ",MergeTraits:{Priority:4294967295}"
                            ",Version:1400050000"
                            ",ShardNumber:1"
                            ",ShardsTotal:2"
                            ",SourceKey:keyA"
                            ",SourceSubkeys:["
                                " {Key:ru,Type:KT_SERP_TLD,Traits:{MustBeInScheme:true,IsPrioritized:true}}"
                                ",{Key:'213',Type:KT_USER_REGION,Traits:{MustBeInScheme:true,IsPrioritized:true}}]"
                            ",Factors:[{StringValue:Aru213,Name:f}]"
                            ",SourceName:test-1"
                            ",TrieName:'test-1.trie.1400050000'"
                        "}]}"
                        ,"{"
                            " 'test-0':{"
                                " timestamp:1400050000"
                                ",values:{"
                                    " keyA:{ru:{'3':{f:Aru3}}}"
                                    ",keyB:{ru:{'10000':{f:Bru10000}}}"
                                    ",keyC:{'*':{'213':{f:'C*213'}}}"
                                "}"
                            "}"
                            ",'test-1':{"
                                " timestamp:1400050000"
                                ",values:{"
                                    " keyA:{ru:{'213':{f:Aru213}}}"
                                    ",keyB:{ru:{'3':{f:Bru3}}}"
                                    ",keyC:{'*':{'1':{f:'C*1'}}}"
                                "}"
                            "}"
                        "}");
    }

};

UNIT_TEST_SUITE_REGISTRATION(TQueryDataServerTest)

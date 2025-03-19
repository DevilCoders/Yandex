#include "trie_search.h"

#include <kernel/querydata/ut_utils/qd_ut_utils.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NFacts;

Y_UNIT_TEST_SUITE(TrieSearch) {
    Y_UNIT_TEST(OneSource) {
        THolder<NQueryData::TQueryDatabase> db(new NQueryData::TQueryDatabase());
        db->AddSource(
            NQueryData::NTests::MakeFakeSourceJson(
                R"|({
                    SourceName:test,
                    SourceKeys:[KT_CATEG,KT_SERP_TLD],
                    Version:1400050000,
                    IndexingTimestamp:1410050000,
                    FileTimestamp:1420050000,
                    HasJson:true,
                    HasKeyRef:1,
                    SourceData:{
                        tinkoff.ru:{
                            'ru':{
                                Json:{result:0, kernel:0.625}
                            }
                        }
                    }
                })|"
            )
        );

        TCredibilityDatabase credDb(std::move(db));
        const auto tinkoffCredibility = credDb.FindCredibility("https://journal.tinkoff.ru/3-ndfl", "ru");
        UNIT_ASSERT_EQUAL(tinkoffCredibility.CredibilityMark, TCredibilityDatabase::ECredibilityMark::NonCredible);
        UNIT_ASSERT_EQUAL(tinkoffCredibility.KernelScore, 0.625f);
    }

    Y_UNIT_TEST(MultipleSources) {
        THolder<NQueryData::TQueryDatabase> db(new NQueryData::TQueryDatabase());
        db->AddSource(
            NQueryData::NTests::MakeFakeSourceJson(
                R"|({
                    SourceName:test,
                    SourceKeys:[KT_CATEG,KT_SERP_TLD],
                    Version:1400050000,
                    IndexingTimestamp:1410050000,
                    FileTimestamp:1420050000,
                    HasJson:true,
                    HasKeyRef:1,
                    SourceData:{
                        wikipedia.org:{
                            'ru':{
                                Json:{result:-1, kernel:0.875}
                            }
                        }
                    }
                })|"
            )
        );

        db->AddSource(
            NQueryData::NTests::MakeFakeSourceJson(
                R"|({
                    SourceName:test,
                    SourceKeys:[KT_SERP_TLD,KT_CATEG_URL],
                    Version:1400050000,
                    IndexingTimestamp:1410050000,
                    FileTimestamp:1420050000,
                    HasJson:true,
                    HasKeyRef:1,
                    SourceData:{
                        by:{
                            'vtb.ru ru.vtb.www/*':{
                                Json:{result:0, kernel:0.75}
                            }
                        }
                    }
                })|"
            )
        );
        db->AddSource(
            NQueryData::NTests::MakeFakeSourceJson(
                R"|({
                    SourceName:test,
                    SourceKeys:[KT_URL,KT_SERP_TLD],
                    Version:1400050000,
                    IndexingTimestamp:1410050000,
                    FileTimestamp:1420050000,
                    HasJson:true,
                    HasKeyRef:1,
                    SourceData:{
                        'vtb.ru/about':{
                            'by':{
                                Json:{result:1, kernel:0.75}
                            }
                        },
                        'ru.wikipedia.org/wiki/Москва':{
                            'ru':{
                                Json:{result:1, kernel:0.875}
                            }
                        }
                    }
                })|"
            )
        );

        TCredibilityDatabase credDb(std::move(db));
        const auto wikiCredibility = credDb.FindCredibility("https://ru.wikipedia.org/wiki/Москва", "ru");
        UNIT_ASSERT_EQUAL(wikiCredibility.CredibilityMark, TCredibilityDatabase::ECredibilityMark::Credible);
        UNIT_ASSERT_EQUAL(wikiCredibility.KernelScore, 0.875f);
        const auto vtbCredibility = credDb.FindCredibility("https://www.vtb.ru/about", "by");
        UNIT_ASSERT_EQUAL(vtbCredibility.CredibilityMark, TCredibilityDatabase::ECredibilityMark::Credible);
        UNIT_ASSERT_EQUAL(vtbCredibility.KernelScore, 0.75f);
    }

    Y_UNIT_TEST(NotFound) {
        THolder<NQueryData::TQueryDatabase> db(new NQueryData::TQueryDatabase());
        db->AddSource(
            NQueryData::NTests::MakeFakeSourceJson(
                R"|({
                    SourceName:test,
                    SourceKeys:[KT_CATEG,KT_SERP_TLD],
                    Version:1400050000,
                    IndexingTimestamp:1410050000,
                    FileTimestamp:1420050000,
                    HasJson:true,
                    HasKeyRef:1,
                    SourceData:{
                        journal.tinkoff.ru:{
                            'ru':{
                                Json:{result:0, kernel:0.555}
                            }
                        }
                    }
                })|"
            )
        );

        TCredibilityDatabase credDb(std::move(db));
        const auto tinkoffCredibility = credDb.FindCredibility("https://journal.tinkoff.com/3-ndfl", "ru");
        UNIT_ASSERT_EQUAL(tinkoffCredibility.CredibilityMark, TCredibilityDatabase::ECredibilityMark::Unknown);
        UNIT_ASSERT_EQUAL(tinkoffCredibility.KernelScore, TCredibilityDatabase::DefaultKernelScore);
    }
}

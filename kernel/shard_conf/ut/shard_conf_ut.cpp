#include <kernel/shard_conf/shard_conf.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/registar.h>

class TShardConfTest : public NUnitTest::TTestBase {
UNIT_TEST_SUITE(TShardConfTest)
    UNIT_TEST(TestBaseSearch)
    UNIT_TEST(TestQuerySearch)
    UNIT_TEST(TestWriter)
    UNIT_TEST(TestShardName)
UNIT_TEST_SUITE_END();
private:

    void TestBaseSearch() {
        TShardConf sc = TShardConf::ParseFromString(NResource::Find("/basesearch.shard.conf"));
        UNIT_ASSERT_VALUES_EQUAL(sc.ShardName, "lp100-013-1457643403");
        UNIT_ASSERT_VALUES_EQUAL(sc.Timestamp.Seconds(), 1458052978u);
        UNIT_ASSERT(sc.ShardNumber);
        UNIT_ASSERT_VALUES_EQUAL(*sc.ShardNumber, 2529u);
        UNIT_ASSERT_VALUES_EQUAL(sc.PrevShardName, "lp100-013-1456746258");
        UNIT_ASSERT_VALUES_EQUAL(sc.Files.size(), 88u);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.front().Name, "indexnavutf_dnorm");
        UNIT_ASSERT(!sc.Files.front().NoCheck);

        UNIT_ASSERT(sc.Files.front().MTime);
        UNIT_ASSERT_VALUES_EQUAL(*sc.Files.front().MTime, 1457870502u);
        UNIT_ASSERT(!sc.Files.front().NoCheckMTime);

        UNIT_ASSERT(sc.Files.front().Size);
        UNIT_ASSERT_VALUES_EQUAL(*sc.Files.front().Size, 2425u);
        UNIT_ASSERT(!sc.Files.front().NoCheckSize);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.front().MD5, "e9da006a4777693c4eb3e4a63f77e8d8");
        UNIT_ASSERT(!sc.Files.front().NoCheckSum);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.back().Name, "scripts/upload-common.mk");
        UNIT_ASSERT(!sc.Files.back().NoCheck);

        UNIT_ASSERT(sc.Files.back().MTime);
        UNIT_ASSERT_VALUES_EQUAL(*sc.Files.back().MTime, 1458052149u);
        UNIT_ASSERT(!sc.Files.back().NoCheckMTime);

        UNIT_ASSERT(sc.Files.back().Size);
        UNIT_ASSERT_VALUES_EQUAL(*sc.Files.back().Size, 554u);
        UNIT_ASSERT(!sc.Files.back().NoCheckSize);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.back().MD5, "07496d0d8d077716df29da6fd133d8b7");
        UNIT_ASSERT(!sc.Files.back().NoCheckSum);

        UNIT_ASSERT_VALUES_EQUAL(sc.InstallScript.size(), 1u);
        UNIT_ASSERT_VALUES_EQUAL(sc.InstallScript.front(), "scripts/upload-search.sh");
    }

    void TestQuerySearch() {
        TShardConf sc = TShardConf::ParseFromString(NResource::Find("/querysearch.shard.conf"));
        UNIT_ASSERT_VALUES_EQUAL(sc.ShardName, "banflt-000-1458208108");
        UNIT_ASSERT_VALUES_EQUAL(sc.Timestamp.Seconds(), 1400000000u);
        UNIT_ASSERT(!sc.ShardNumber);
        UNIT_ASSERT(!sc.PrevShardName);
        UNIT_ASSERT_VALUES_EQUAL(sc.Files.size(), 82u);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.front().Name, "ags_test.trie");
        UNIT_ASSERT(!sc.Files.front().NoCheck);

        UNIT_ASSERT(!sc.Files.front().MTime);
        UNIT_ASSERT(sc.Files.front().NoCheckMTime);

        UNIT_ASSERT(sc.Files.front().Size);
        UNIT_ASSERT_VALUES_EQUAL(*sc.Files.front().Size, 29885u);
        UNIT_ASSERT(!sc.Files.front().NoCheckSize);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.front().MD5, "03a1efe232d9ef515603fe26a55715f7");
        UNIT_ASSERT(!sc.Files.front().NoCheckSum);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.back().Name, "youtube.trie");
        UNIT_ASSERT(!sc.Files.back().NoCheck);

        UNIT_ASSERT(!sc.Files.back().MTime);
        UNIT_ASSERT(sc.Files.back().NoCheckMTime);

        UNIT_ASSERT(sc.Files.back().Size);
        UNIT_ASSERT_VALUES_EQUAL(*sc.Files.back().Size, 205741u);
        UNIT_ASSERT(!sc.Files.back().NoCheckSize);

        UNIT_ASSERT_VALUES_EQUAL(sc.Files.back().MD5, "d2c2a979a7fca0798321a906c1a5585d");
        UNIT_ASSERT(!sc.Files.back().NoCheckSum);

        UNIT_ASSERT(!sc.InstallScript);
    }

    void TestWriter() {
        {
            TShardConf sc(
                    TInstant::Seconds(1000050002),
                    "my-shard-000-1000050000",
                    "my-shard-000-1000049999",
                    {{"file.txt", "f0f0f0", 1000050001, 11}},
                    {"script.sh"}
            );

            UNIT_ASSERT_STRINGS_EQUAL(TShardConf::ParseFromString(sc.ToString()).ToString(),
                                     "%shard my-shard-000-1000050000\n"
                                     "%mtime 1000050002\n"
                                     "%requires my-shard-000-1000049999\n"
                                     "%files\n"
                                     "%attr(mtime=1000050001, md5=f0f0f0, size=11) file.txt\n"
                                     "%install\n"
                                     "script.sh\n"
                                     "\n");
        }
        {
            TShardConf sc;
            sc.InstallSize = 100;
            UNIT_ASSERT_STRINGS_EQUAL(TShardConf::ParseFromString(sc.ToString()).ToString(),
                                      "%files\n%install_size 100\n");
        }
        {
            TShardConf sc;
            UNIT_ASSERT_STRINGS_EQUAL(TShardConf::ParseFromString(
                    TShardConf::ParseFromString("%files\nfoo.txt\nbar.txt\n").ToString()).ToString(),
                                      "%files\n"
                                      "%attr(nocheckmtime, nochecksum, nochecksize) foo.txt\n"
                                      "%attr(nocheckmtime, nochecksum, nochecksize) bar.txt\n");
        }
        {
            UNIT_ASSERT_STRINGS_EQUAL(TShardConf::ParseFromString("%shard my-shard-000-1000050000# some # comment").ToString(),
                                      "%shard my-shard-000-1000050000\n%files\n");
        }
        {
            TShardConf sc(
                    TInstant::Seconds(1000050002),
                    "my-shard-000-1000050000",
                    "",
                    {{"file.txt", "f0f0f0", 1000050001, 11}},
                    {"script.sh"}
            );

            UNIT_ASSERT_STRINGS_EQUAL(TShardConf::ParseFromString(sc.ToString()).ToString(),
                                     "%shard my-shard-000-1000050000\n"
                                     "%mtime 1000050002\n"
                                     "%files\n"
                                     "%attr(mtime=1000050001, md5=f0f0f0, size=11) file.txt\n");
        }
    }

    void TestShardName() {
        TShardName sn = TShardName::ParseFromString("my-shard-001-1000050000");
        UNIT_ASSERT_VALUES_EQUAL(sn.Name, "my-shard");
        UNIT_ASSERT_VALUES_EQUAL(sn.Number, 1u);
        UNIT_ASSERT_VALUES_EQUAL(sn.Timestamp, 1000050000u);
        UNIT_ASSERT_VALUES_EQUAL(TShardName("xxx", 0, 0).ToString(), "xxx-000-0000000000");
        UNIT_CHECK_GENERATED_EXCEPTION(TShardName::ParseFromString("my-shard-1-1000050000"), yexception);
        UNIT_CHECK_GENERATED_EXCEPTION(TShardName::ParseFromString("my-shard-001"), yexception);
        UNIT_CHECK_GENERATED_EXCEPTION(TShardName::ParseFromString("my-sh\tard-001-1000050000"), yexception);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TShardConfTest);

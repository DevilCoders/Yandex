#include <library/cpp/testing/unittest/registar.h>

#include "robots_txt.h"

class TRobotsTxtTest: public TTestBase {
    UNIT_TEST_SUITE(TRobotsTxtTest);
    //    UNIT_TEST(TestRobotsBazil7)
    //    UNIT_TEST(TestRobotsMemory)
    UNIT_TEST(TestRobotsBazil)
    UNIT_TEST(TestRobotsBazil2)
    UNIT_TEST(TestRobotsBazil3)
    UNIT_TEST(TestRobotsBazil4)
    UNIT_TEST(TestRobotsBazil5)
    UNIT_TEST(TestRobotsBazil6)
    UNIT_TEST(TestRobotsBazil8)
    UNIT_TEST(TestRobotsBazil9)
    UNIT_TEST(TestRobotsBazil10)
    UNIT_TEST(TestRobotsBazil11)
    UNIT_TEST(TestRobotsBazil12)
    UNIT_TEST(TestRobotsBazil13)
    UNIT_TEST(TestRobotsBazil14)
    UNIT_TEST(TestRobotsBazil15)
    UNIT_TEST(TestRobotsPanda1)
    UNIT_TEST(TestRobotsPanda2)
    UNIT_TEST(TestRobotsPanda3)
    UNIT_TEST(TestRobotsPanda4)
    UNIT_TEST(TestRobotsPanda5)
    UNIT_TEST(TestRobotsPanda6)
    UNIT_TEST(TestRobotsPanda7)
    UNIT_TEST(TestRobotsPanda8)
    UNIT_TEST(TestRobotsPanda9)
    UNIT_TEST(TestRobotsPanda10)
    UNIT_TEST(TestRobotsPanda11)
    UNIT_TEST(TestRobotsPanda12)
    UNIT_TEST(TestRobotsPanda13)
    UNIT_TEST(TestRobotsPanda14)
    UNIT_TEST(TestRobotsPanda15)
    UNIT_TEST(TestRobotsPanda16)
    UNIT_TEST(TestRobotsPanda17)
    UNIT_TEST(TestRobotsPanda18)
    UNIT_TEST(TestRobotsPanda19)
    UNIT_TEST(TestRobotsPanda20)
    UNIT_TEST(TestRobotsPanda21)
    UNIT_TEST(TestRobotsPanda22)
    UNIT_TEST(TestRobotsPanda23)
    UNIT_TEST(TestRobotsPanda24)
    UNIT_TEST(TestRobotsPanda25)
    UNIT_TEST(TestRobotsPanda26)
    UNIT_TEST(TestRobotsPanda27)
    UNIT_TEST(TestRobotsPanda28)
    UNIT_TEST(TestRobotsQuick1)
    UNIT_TEST(TestRobotsQuick2)
    UNIT_TEST(TestRobotsQuick3)
    UNIT_TEST(TestRobotsQuick4)
    UNIT_TEST(TestRobotsQuick5)
    UNIT_TEST(TestRobotsQuick6)
    UNIT_TEST(TestRobotsQuick7)
    UNIT_TEST(TestRobotsQuick8)
    UNIT_TEST(TestRobotsQuick9)
    UNIT_TEST(TestRobotsQuick10)
    UNIT_TEST(TestRobotsQuick11)
    UNIT_TEST(TestRobotsQuick12)
    UNIT_TEST(TestRobotsQuick13)
    UNIT_TEST(TestRobotsQuick14)
    UNIT_TEST(TestRobotsQuick15)
    UNIT_TEST(TestRobotsQuick16)
    UNIT_TEST(TestRobotsQuick17)
    UNIT_TEST(TestRobotsQuick18)
    UNIT_TEST(TestRobotsQuick19)
    UNIT_TEST(TestRobotsQuick20)
    UNIT_TEST(TestRobotsQuick21)
    UNIT_TEST(TestRobotsQuick22)
    UNIT_TEST(TestRobotsQuick23)
    UNIT_TEST(TestRobotsQuick24)
    UNIT_TEST(TestRobotsCrawlDelayForDisallowAll)
    UNIT_TEST(TestRobotsCrawlDelayForDisallowAll2)
    UNIT_TEST(TestRobotsMultipleBlocks)
    UNIT_TEST(TestRobotsBOM)
    UNIT_TEST(TestRobotsIvi)
    UNIT_TEST(TestRobotsRestore)
    UNIT_TEST_SUITE_END();

private:
    inline void TestRobotsIvi() {
        TString s = ""
            "User-agent: *\n"
            "Allow: /*/player?*\n"
            "Allow: /css/\n"
            "Allow: *.css\n"
            "Allow: /scripts/\n"
            "Disallow: /search/\n"
            "Disallow: /temporary/*\n"
            "Disallow: /mobileapi/*\n"
            "Disallow: /format/*\n"
            "Disallow: /speedtest/*\n"
            "Disallow: /external/*\n"
            "Disallow: /embeds/*\n"
            "Disallow: /branding/*\n"
            "Disallow: /*?*\n"
            "Disallow: /*start=\n"
            "Disallow: /*finish=\n"
            "Disallow: /partner/*\n"
            "Disallow: /profile*\n"
            "Disallow: /unsubscribe/*\n"
            "Disallow: /buy*\n"
            "Disallow: */filters/*\n"
            "Sitemap: https://www.ivi.tv/sitemap.xml\n"
            "\n"
            "\n"
            "User-agent: Yandex\n"
            "Disallow: *\n"
            "\n";

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        UNIT_ASSERT(robots.IsDisallow(robotstxtcfg::id_yandexbot, "/movies"));
    }

    inline void TestRobotsBazil() {
        TString s =
            TString("User-agent: *\n") +
            TString("Allow: /foo\n") +
            TString("Disallow: /\n") +
            TString("Crawl-delay: 3\n") +
            TString("Host: www.somehost.com\n") +
            TString("Clean-param: s /forum/showthread.php\n") +
            TString("\n") +
            TString("Sitemap: http://otherhost2.net/sitemap.xml.gz\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Allow: /bar\n") +
            TString("Disallow: /foo\n") +
            TString("Allow: /\n") +
            TString("Crawl-delay: 20\n") +
            TString("Clean-param: sid&sort /forumt/*.php\n") +
            TString("Clean-param: someTrash&otherTrash\n") +
            TString("\n") +
            TString("User-agent: YandexBlogs\n") +
            TString("Allow: /\n") +
            TString("Crawl-delay: 5\n") +
            TString("Sitemap: http://otherhost.net/sitemap.xml\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexblogsbot);
        botIds.insert(robotstxtcfg::id_msnbot);
        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        TVector<int> acceptedLines = robots.GetAcceptedLines();
        //Cout << "Size: " << acceptedLines.size() << Endl;
        //for (ui32 i = 0; i < acceptedLines.size(); ++i)
        //    Cout << "Line: " << acceptedLines[i] << Endl;
        UNIT_ASSERT_EQUAL(acceptedLines.size(), 11);
        UNIT_ASSERT_EQUAL(acceptedLines[0], 5);
        UNIT_ASSERT_EQUAL(acceptedLines[1], 6);
        UNIT_ASSERT_EQUAL(acceptedLines[2], 8);
        UNIT_ASSERT_EQUAL(acceptedLines[3], 10);
        UNIT_ASSERT_EQUAL(acceptedLines[4], 11);
        UNIT_ASSERT_EQUAL(acceptedLines[5], 12);
        UNIT_ASSERT_EQUAL(acceptedLines[6], 13);
        UNIT_ASSERT_EQUAL(acceptedLines[7], 14);
        UNIT_ASSERT_EQUAL(acceptedLines[8], 15);
        UNIT_ASSERT_EQUAL(acceptedLines[9], 16);
        UNIT_ASSERT_EQUAL(acceptedLines[10], 21);

        for (ui32 botId = 0; botId < robotstxtcfg::max_botid; ++botId) {
            UNIT_ASSERT_EQUAL(robots.IsBotIdLoaded(botId), robots2.IsBotIdLoaded(botId));
            //            if (robots.IsBotIdLoaded(botId)) {
            if (botId == robotstxtcfg::id_anybot) {
                UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(botId), 3000);
                UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(botId), 3000);
            } else if (botId == robotstxtcfg::id_yandexbot) {
                UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(botId), 20000);
                UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(botId), 20000);
            } else if (botId == robotstxtcfg::id_msnbot) {
                UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(botId), 3000);
                UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(botId), 3000);
            } else if (botId == robotstxtcfg::id_yandexblogsbot) {
                UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(botId), 5000);
                UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(botId), 5000);
            }

            //            }
        }

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(), 5000);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), 5000);

        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "www.somehost.com");
        UNIT_ASSERT_EQUAL(robots2.GetHostDirective(), "www.somehost.com");

        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 3);
        UNIT_ASSERT_EQUAL(robots2.GetCleanParams().size(), 3);
        for (ui32 i = 0; i < 3; ++i)
            UNIT_ASSERT_EQUAL(robots.GetCleanParams()[i], robots2.GetCleanParams()[i]);

        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 2);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 2);
        for (ui32 i = 0; i < 2; ++i) {
            UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[i], robots2.GetSiteMaps()[i]);
        }

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), false);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/foo2");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/foo2");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsAllow(robotstxtcfg::id_yandexbot, "/xxx");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllow(robotstxtcfg::id_yandexbot, "/xxx");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsAllow(robotstxtcfg::id_msnbot, "/foobar");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllow(robotstxtcfg::id_msnbot, "/foobar");
        UNIT_ASSERT_EQUAL(compare, true);

        robots.DoAllowAll();
        robotsPackedSize = robots.GetPacked(robotsPacked);
        robots2.LoadPacked(robotsPacked);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "www.somehost.com");
        UNIT_ASSERT_EQUAL(robots2.GetHostDirective(), "www.somehost.com");

        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 3);
        UNIT_ASSERT_EQUAL(robots2.GetCleanParams().size(), 3);
        for (ui32 i = 0; i < 3; ++i)
            UNIT_ASSERT_EQUAL(robots.GetCleanParams()[i], robots2.GetCleanParams()[i]);

        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 2);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 2);
        for (ui32 i = 0; i < 2; ++i) {
            UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[i], robots2.GetSiteMaps()[i]);
        }
    }

    inline void TestRobotsPanda1() {
        TString s =
            TString("User-agent: Mediapartners-Google\n") +
            TString("Disallow: \n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /search\n") +
            TString("\n") +
            TString("Sitemap: http://catmalojian.blogspot.com/feeds/posts/default?orderby=updated\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/search/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/search/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/testurl/search ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/testurl/search ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://catmalojian.blogspot.com/feeds/posts/default?orderby=updated");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://catmalojian.blogspot.com/feeds/posts/default?orderby=updated");
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/search/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/search/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/testurl/search ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/testurl/search ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_googlebot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), -1);
    }

    inline void TestRobotsPanda2() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /*/feed/\n") +
            TString("Disallow: /*/trackback/\n") +
            TString("\n") +
            TString("# BEGIN XML-SITEMAP-PLUGIN\n") +
            TString("Sitemap: http://blog.rv.net/sitemap.xml.gz\n") +
            TString("# END XML-SITEMAP-PLUGIN\n") +
            TString("\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexblogsbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://blog.rv.net/sitemap.xml.gz");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://blog.rv.net/sitemap.xml.gz");
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/puti/trackback/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/puti/trackback/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/puti/feed/feed1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/puti/feed/feed1 ");
        UNIT_ASSERT_EQUAL(compare, true);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);
    }

    inline void TestRobotsPanda3() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /click?\n") +
            TString("Disallow: /?epl=\n") +
            TString("Disallow: /*?epl=\n") +
            TString("Disallow: /*?*&epl=\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_msnbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/param?epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/param?epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/param?param&epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/param?param&epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexdirectbot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexdirectbot), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/param?epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/param?epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/param?param&epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/param?param&epl=1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_msnbot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_msnbot), -1);
    }

    inline void TestRobotsPanda4() {
        TString s =
            TString("# For domain: http://www.verdict.by/\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Disallow: /dir\n") +
            TString("\n") +
            TString("Disallow: /dir2\n") +
            TString("\n") +
            TString("Disallow: /dir3\n") +
            TString("\n") +
            TString("Crawl-delay: 2\n") +
            TString("Host: www.verdict.by\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_anybot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 2000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 2000);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "www.verdict.by");
    }

    inline void TestRobotsPanda5() {
        TString s =
            TString("Sitemap: http://crystiano.wordpress.com/sitemap.xml\n") +
            TString("\n") +
            TString("User-agent: IRLbot\n") +
            TString("Crawl-delay: 3600\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /next/\n") +
            TString("\n") +
            TString("# har har\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /activate/\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /signup/\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /related-tags.php\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow:\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_msnbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://crystiano.wordpress.com/sitemap.xml");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://crystiano.wordpress.com/sitemap.xml");
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/testsearch ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/testsearch ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/next/testsearch ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/next/testsearch ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/signup/test");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/signup/test ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/testsearch ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/testsearch ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/next/testsearch ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/next/testsearch ");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsPanda6() {
        TString s =
            TString("User-agent: *\n") +
            TString("crawl-delay: 60\n") +
            TString("Disallow: /display/TEST\n") +
            TString("Disallow: /display/TESTING\n") +
            TString("Disallow: /label\n") +
            TString("Disallow: /pages/doexportpage.action\n") +
            TString("Disallow: /exportword\n") +
            TString("Disallow: /spaces/flyingpdf\n") +
            TString("Disallow: /pages/downloadallattachments.action\n") +
            TString("Disallow: /download/temp/\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yahooslurp);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/label ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/label ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/lab ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/lab ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_anybot), 60000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), 60000);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "");
        UNIT_ASSERT_EQUAL(robots2.GetHostDirective(), "");
        compare = robots.IsDisallow(robotstxtcfg::id_yandexcatalogbot, "/pages/doexportpage.action/action ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexcatalogbot, "/pages/doexportpage.action/action ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexcatalogbot, "/export ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexcatalogbot, "/export ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexcatalogbot), 60000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexcatalogbot), 60000);
        compare = robots.IsDisallow(robotstxtcfg::id_yahooslurp, "/display/TESTING/test ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yahooslurp, "/display/TESTING/test ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yahooslurp, "/display/TES/TEST ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yahooslurp, "/display/TES/TEST ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yahooslurp), 60000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yahooslurp), 60000);
    }

    inline void TestRobotsPanda7() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow:\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexmediabot);
        botIds.insert(robotstxtcfg::id_yahooslurp);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexmediabot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexmediabot), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexmediabot, "/urltest ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexmediabot, "/urltest ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yahooslurp), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yahooslurp), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_yahooslurp, "/urltest ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yahooslurp, "/urltest ");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    inline void TestRobotsPanda8() {
        TString s =
            TString("User-agent: *\n") +
            TString("Allow: /\n") +
            TString("Disallow: /cgi-bin\n") +
            TString("Disallow: /wp-admin\n") +
            TString("Disallow: /wp-includes\n") +
            TString("Disallow: /wp-content\n") +
            TString("Disallow: /search/*/feed\n") +
            TString("Disallow: /search/*/*\n") +
            TString("\n") +
            TString("User-agent: Mediapartners-Google\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: Adsbot-Google\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: Googlebot-Image\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: Googlebot-Mobile\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("#User-agent: ia_archiver-web.archive.org\n") +
            TString("#Disallow: /\n") +
            TString("\n") +
            TString("Sitemap: http://exerciseballabs.com/sitemap.xml\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/wp-includes ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/wp-includes ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://exerciseballabs.com/sitemap.xml");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://exerciseballabs.com/sitemap.xml");
        compare = robots.IsDisallow(robotstxtcfg::id_googlebot, "/search/pram/feed/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_googlebot, "/search/pram/feed/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_googlebot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), -1);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://exerciseballabs.com/sitemap.xml");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://exerciseballabs.com/sitemap.xml");
    }

    inline void TestRobotsPanda9() {
        TString s =
            TString("Host: lib.ru\n") +
            TString("Crawl-delay: 666\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /cgi-bin/ratings.pl\n") +
            TString("Disallow: /fsearch.html\n") +
            TString("Disallow: /wiki.html?h=\n") +
            TString("Disallow: /fcluster/gotopost.pl\n") +
            TString("Disallow: /images/\n") +
            TString("\n") +
            TString("User-agent: Slurp\n") +
            TString("Crawl-delay: 20\n") +
            TString("\n") +
            TString("User-agent: Google\n") +
            TString("Crawl-delay: 10\n") +
            TString("\n") +
            TString("User-agent: msn\n") +
            TString("Crawl-delay: 30 \n") +
            TString("Host: lib2.ru\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Host: lib3.ru\n") +
            TString("Disallow: /cgi-bin/ratings.pl\n") +
            TString("Disallow: /fsearch.html\n") +
            TString("Disallow: /wiki.html?h=\n") +
            TString("Disallow: /fcluster/gotopost.pl\n") +
            TString("Disallow: /images/\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yahooslurp);
        botIds.insert(robotstxtcfg::id_msnbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 0);
        compare = robots.IsDisallow(robotstxtcfg::id_googlebot, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_googlebot, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_googlebot, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_googlebot, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_googlebot), 10000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), 10000);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 0);
        compare = robots.IsDisallow(robotstxtcfg::id_yahooslurp, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yahooslurp, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yahooslurp, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yahooslurp, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yahooslurp), 20000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yahooslurp), 20000);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 0);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/wiki.html?h=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/wiki.html?hh=param ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_msnbot), 30000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_msnbot), 30000);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 0);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 0);

        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "lib.ru");
        UNIT_ASSERT_EQUAL(robots2.GetHostDirective(), "lib.ru");
    }

    inline void TestRobotsPanda10() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /wp-content/\n") +
            TString("Disallow: /wp-includes/\n") +
            TString("Disallow: /wp-admin/\n") +
            TString("Disallow: /images/\n") +
            TString("Disallow: /wp-login.php\n") +
            TString("Disallow: /wp-register.php\n") +
            TString("Disallow: /index.php?s=\n") +
            TString("Disallow: /xmlrpc.php\n") +
            TString("Host: finance.blogrange.com\n") +
            TString("\n") +
            TString("Sitemap: http://finance.blogrange.com/sitemap.xml.gz\n") +
            TString("Sitemap: http://finance.blogrange.com/sitemap2.xml.gz\n") +
            TString("Sitemap: http://finance.blogrange.com/sitemap3.xml.gz\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Crawl-delay: 1\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/images/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/images/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_anybot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/images/ ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/images/ ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://finance.blogrange.com/sitemap.xml.gz");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://finance.blogrange.com/sitemap.xml.gz");
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 1000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 1000);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "finance.blogrange.com");
    }

    inline void TestRobotsPanda11() {
        TString s =
            TString("# robots.txt to block all bots except bots from Google , MSN , Yahoo\n") +
            TString("User-agent: Googlebot\n") +
            TString("Disallow:\n") +
            TString("User-agent: Slurp\n") +
            TString("Disallow:\n") +
            TString("User-agent: MSNBot\n") +
            TString("Disallow:\n") +
            TString("User-agent: ia_archiver\n") +
            TString("Disallow:\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_msnbot);
        botIds.insert(robotstxtcfg::id_yahooslurp);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_googlebot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_googlebot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_msnbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_msnbot, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yahooslurp, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yahooslurp, "/url ");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    inline void TestRobotsPanda12() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /*lang=eng\n") +
            TString("Disallow: /*order_by\n") +
            TString("Disallow: /statistics/\n") +
            TString("Disallow: /info/\n") +
            TString("Disallow: /test/\n") +
            TString("Disallow: /partners/\n") +
            TString("Disallow: /albums/\n") +
            TString("Disallow: /groups/\n") +
            TString("Disallow: /hobby/\n") +
            TString("Disallow: /user/\n") +
            TString("Disallow: /profile/\n") +
            TString("Disallow: /reklama/\n") +
            TString("Disallow: /includes/\n") +
            TString("Disallow: /vacancy/search/\n") +
            TString("Disallow: /vacancy/friend_vacancy.html\n") +
            TString("Disallow: /vacancy/archive/\n") +
            TString("Disallow: /resume/search_resume.html\n") +
            TString("Disallow: /resume/search_region.html\n") +
            TString("Disallow: /resume/friend_resume.html\n") +
            TString("Disallow: /resume/send_resume.html\n") +
            TString("Disallow: /resume/views.html\n") +
            TString("Disallow: /hr/\n") +
            TString("Disallow: /info/checkup/\n") +
            TString("Disallow: /info/read.html\n") +
            TString("Disallow: /info/send_cookies.html\n") +
            TString("Disallow: /info/set_simple_auth_cookie.html\n") +
            TString("Disallow: /info/show_mail_return_reason.html\n") +
            TString("Disallow: /trudovoj_kodeks_tk_ua/\n") +
            TString("Disallow: /partners/\n") +
            TString("Disallow: /statistics/\n") +
            TString("Disallow: /registration.html\n") +
            TString("Disallow: /functions/\n") +
            TString("Disallow: /cgi-bin/\n") +
            TString("Disallow: /files/\n") +
            TString("Disallow: /images/\n") +
            TString("Disallow: /w3timages/\n") +
            TString("Disallow: /rus/\n") +
            TString("Disallow: /banners/\n") +
            TString("Disallow: /zarplatomer/\n") +
            TString("Disallow: /test/\n") +
            TString("Disallow: /horoscope/\n") +
            TString("Disallow: /athens/\n") +
            TString("Disallow: /static/\n") +
            TString("Disallow: /cito/\n") +
            TString("Disallow: /community/message/\n") +
            TString("Disallow: /feedback/\n") +
            TString("Disallow: /print/\n") +
            TString("Disallow: /community/chat/\n") +
            TString("Disallow: /community/marketing/\n") +
            TString("Disallow: /community/politics/\n") +
            TString("Disallow: /community/accountant/\n") +
            TString("Disallow: /community/it/\n") +
            TString("Disallow: /community/autoclub/\n") +
            TString("Disallow: /community/contests/\n") +
            TString("Disallow: /community/mw/\n") +
            TString("Disallow: /community/assistant/\n") +
            TString("Disallow: /community/fraud/\n") +
            TString("Disallow: /community/sale/\n") +
            TString("Disallow: /community/edu/\n") +
            TString("Disallow: /community/building/\n") +
            TString("Disallow: /community/cinema/\n") +
            TString("Disallow: /community/humor/\n") +
            TString("Disallow: /community/cats/\n") +
            TString("Disallow: /community/adv/\n") +
            TString("Disallow: /community/design/\n") +
            TString("Disallow: /catalog/\n") +
            TString("Host: pavlograd.superjob.ua\n") +
            TString("\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: CazoodleBot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: WebInject\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: IRLbot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: gigabot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: TurnitinBot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: psbot\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/print/printpage ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/print/printpage ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/print/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/print/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/print ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/print ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandeximagesbot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandeximagesbot), -1);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "pavlograd.superjob.ua");
    }

    inline void TestRobotsPanda13() {
        TString s =
            TString("User-agent: YandexBlog\n") +
            TString("Clean-param: sid /index.php\n") +
            TString("Disallow:\n") +
            TString("Clean-param: gidd&xxx /viewforum.php\n") +
            TString("User-agent: *\n") +
            TString("Allow: /wp-content/uploads/\n") +
            TString("Disallow: /wp-content\n") +
            TString("Disallow: /?s=\n") +
            TString("Sitemap: http://harch.ru/sitemap.xml\n") +
            TString("Clean-param: sid /index.php\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexblogsbot);
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/?s=test ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/?s=test ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/?s=test ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/?s=test ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/wp-content/uploads/test ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/wp-content/uploads/test ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/wp-content ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/wp-content ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://harch.ru/sitemap.xml");
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps()[0], "http://harch.ru/sitemap.xml");

        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 2);
        UNIT_ASSERT_EQUAL(robots2.GetCleanParams().size(), 2);
        for (ui32 i = 0; i < 2; ++i)
            UNIT_ASSERT_EQUAL(robots.GetCleanParams()[i], robots2.GetCleanParams()[i]);

        UNIT_ASSERT_EQUAL(robots.GetCleanParams()[0], "gidd&xxx /viewforum\\.php.*");
        UNIT_ASSERT_EQUAL(robots.GetCleanParams()[1], "sid /index\\.php.*");
    }

    inline void TestRobotsPanda14() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /wp-*\n") +
            TString("#Disallow: /category\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexpagechk);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/wp- ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/wp- ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/category/caturl ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/category/caturl ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexpagechk, "/wp-/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexpagechk, "/wp-/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexpagechk, "/wp-/wpurl ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexpagechk, "/wp-/wpurl ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexpagechk, "/wp ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexpagechk, "/wp ");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    inline void TestRobotsPanda15() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 6\n") +
            TString("\n") +
            TString("User-Agent: YaDirectFetcher \n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 2\n") +
            TString("\n") +
            TString("User-Agent: Google\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 3\n") +
            TString("\n") +
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 9\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_anybot), 6000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), 6000);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 9000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 9000);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexdirectbot), 2000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexdirectbot), 2000);
        compare = robots.IsDisallow(robotstxtcfg::id_googlebot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_googlebot, "/shop/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_googlebot), 3000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), 3000);
    }

    inline void TestRobotsPanda16() {
        TString s =
            TString("User-agent: *\n") +
            TString("Crawl-delay: 821\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexnewsbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_anybot), 821000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), 821000);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 821000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 821000);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexnewsbot), 821000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexnewsbot), 821000);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexnewsbot, "/content ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexnewsbot, "/content ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_googlebot), 821000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), 821000);
    }

    inline void TestRobotsPanda17() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /shop1\n") +
            TString("\n") +
            TString("User-agent: YandexBlog\n") +
            TString("Disallow: /shop2\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Disallow: /shop3\n") +
            TString("\n") +
            TString("User-agent: YandexBot\n") +
            TString("Disallow: /shop4\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_yandexblogsbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;

        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop1/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop2/1 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop3/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexblogsbot, "/shop4/1 ");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    inline void TestRobotsPanda18() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Disallow: /print\n") +
            TString("Host: site.ru\n") +
            TString("Disallow: /print2\n") +
            TString("\n") +
            TString("Disallow: /print3\n") +
            TString("Host: www.site.ru\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbotmirr);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "site.ru");
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbotmirr, "/print3/3 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbotmirr, "/print3/3 ");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsPanda19() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 6\n") +
            TString("\n") +
            TString("User-Agent: YaDirectFetcher \n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 2\n") +
            TString("\n") +
            TString("User-Agent: Google\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 3\n") +
            TString("\n") +
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 9\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(), 2000);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), 2000);
    }

    inline void TestRobotsPanda20() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 6\n") +
            TString("\n") +
            TString("User-Agent: Google\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 3\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(), 6000);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), 6000);
    }

    inline void TestRobotsPanda21() {
        TString s =
            TString("User-Agent: Google\n") +
            TString("Disallow: /shop/\n") +
            TString("Crawl-delay: 3\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(), -1);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), -1);
    }

    inline void TestRobotsBazil2() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-Agent: Google\n") +
            TString("Disallow: /profile\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), true);
    }

    inline void TestRobotsBazil3() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /aaa\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);
    }

    inline void TestRobotsBazil4() {
        TString s =
            TString("User-agent: *\n") +
            TString("Allow: /comments\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2;
        robots2.LoadPacked(robotsPacked);

        TVector<int> acceptedLines = robots.GetAcceptedLines();
        UNIT_ASSERT_EQUAL(acceptedLines.size(), 3);
        for (ui32 i = 0; i < acceptedLines.size(); ++i)
            UNIT_ASSERT_EQUAL((ui32)acceptedLines[i], i + 1);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);
    }

    inline void TestRobotsBazil5() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /shop1/\n") +
            TString("Crawl-delay: 6\n") +
            TString("\n") +
            TString("User-Agent: YaDirectFetcher \n") +
            TString("Disallow: /shop2/\n") +
            TString("Crawl-delay: 2\n") +
            TString("\n") +
            TString("User-Agent: Google\n") +
            TString("Disallow: /shop3/\n") +
            TString("Crawl-delay: 3\n") +
            TString("\n") +
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /shop4/\n") +
            TString("Crawl-delay: 9\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TRobotsTxt robots;
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2;
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop1/ ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop1/ ");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2/ ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2/ ");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop3/ ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop3/ ");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop4/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop4/ ");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsBazil6() {
        TString s =
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/?*\n") +
            TString("Disallow: /compare/\n") +
            TString("Disallow: /basket/\n") +
            TString("Disallow: /akcii/\n") +
            TString("Disallow: /katalog_tovarov/vip_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/ekonom_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/srednie_bukety/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/obychnye_bukety/\n") +
            TString("Disallow: /*/svadebnye_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_cvetu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_29_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/korporativ1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/bolee_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/s_pribavleniem/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/lublu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/skuhayu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosti/prosti/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_razmeru/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/serenada/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/ustoma/bukety_iz_eustom_i_drugih_cvetov/pautinka1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/belosnezhnyj_kupol/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/ekaterina/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz_i_drugih_cvetov/pushistik/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/gloriya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/podarok_iz_roz1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/klubnichnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/lesnoe_chudo/\n") +
            TString("Disallow: /katalog_tovarov/korziny_iz_cvetov1/korziny_iz_raznih_cvetov/korzina_s_liliyami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/dolores_s_kallami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/dolores_s_kallami1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/rozovye_rozy/buket_iz_21_rozovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/zheltye_rozy/serdce_iz_zheltyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zhemchuzhinka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/igrivyj/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/nezhnye_chuvstva/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/miks/raduga_iz_101_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem_i_drugih_cvetov/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/irisovaya_prelest/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/korzina_s_rozami_miks/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/ocharovanie/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/prichuda/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/frezii/bukety_iz_frezij_i_drugih_cvetov/belochka2/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gerbers/bukety_iz_gerber_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/krasnyj_kardinal/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/alla/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/bol_shaya_radost/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/skuchayu_po_tebe/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_orhidey/bravissimo2/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/prelest/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_hrizantemami/prelest1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/neobychnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_21_oranzhevoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/yubilej/konstanciya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/alenushka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_11_belyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/na_schast_e1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/kloun/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/lilies/bukety_iz_lilij/nezabyvaemyj_den2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_kremovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_eustomami/utrennyaya_roza1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zharkoe_leto/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zajchonok1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/populyarnye_bukety1/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem/oduvanchik/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/karameliya/\n") +
            TString("Disallow: /cart/\n") +
            TString("Disallow: /emarket/\n") +
            TString("Disallow: /katalog_tovarov/segodnya_dostavili/\n") +
            TString("Host: dailyflowers.ru\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yandexmediabot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        /*
        botIds.insert(robotstxtcfg::id_yandexbotmirr);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_yandexblogsbot);
        botIds.insert(robotstxtcfg::id_yandexnewsbot);
        botIds.insert(robotstxtcfg::id_yandexpagechk);
        */

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/kustovye_rozy/");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/kustovye_rozy/");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/?param=xxx");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/?param=xxx");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsBazil8() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Disallow: /cgi-bin/\n") +
            TString("Disallow: /edu/admin/\n") +
            TString("Disallow: /edu/admin_/\n") +
            TString("Disallow: /edu/admin__/\n") +
            TString("Disallow: /edu/admint/\n") +
            TString("Disallow: /admin/\n") +
            TString("Disallow: /transport/\n") +
            TString("Disallow: /index.php?id=search\n") +
            TString("Disallow: /search/\n") +
            TString("Disallow: /edu/search/\n") +
            TString("Disallow: /edu/about/news/microsoft/\n") +
            TString("Disallow: /edu/order/\n") +
            TString("Disallow: /edu/certification/*/index.php\n") +
            TString("Disallow: /edu/study/cisco/index.php$\n") +
            TString("Disallow: /edu/study/cisco/$\n") +
            TString("Disallow: /edu/study/*/index.php?c=1\n") +
            TString("Disallow: /edu/study/microsoft/$\n") +
            TString("Disallow: /edu/study/microsoft/index.php$\n") +
            TString("Crawl-delay: 3\n") +
            TString("Clean-param: PHPSESSID *\n") +
            TString("Host: www.eureca.ru\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /cgi-bin/\n") +
            TString("Disallow: /edu/admin/\n") +
            TString("Disallow: /edu/admin_/\n") +
            TString("Disallow: /edu/admin__/\n") +
            TString("Disallow: /edu/admint/\n") +
            TString("Disallow: /admin/\n") +
            TString("Disallow: /transport/\n") +
            TString("Disallow: /index.php?id=search\n") +
            TString("Disallow: /search/\n") +
            TString("Disallow: /edu/search/\n") +
            TString("Disallow: /edu/about/news/microsoft/\n") +
            TString("Disallow: /edu/order/\n") +
            TString("Disallow: /edu/study/microsoft/$\n") +
            TString("Disallow: /edu/study/microsoft/index.php$\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        TVector<int> acceptedLines = robots.GetAcceptedLines();
        UNIT_ASSERT_EQUAL(acceptedLines.size(), 22);
        for (int i = 0; i < 22; ++i)
            UNIT_ASSERT_EQUAL(acceptedLines[i], i + 1);
    }

    inline void TestRobotsBazil9() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Disallow: /cgi-bin/\r") +
            TString("Disallow: /edu/admin/\n") +
            TString("Disallow: /edu/admin_/\n") +
            TString("Disallow: /edu/admin__/\n") +
            TString("Disallow: /edu/admint/\n") +
            TString("Disallow: /admin/\n") +
            TString("Disallow: /transport/\r\n") +
            TString("Disallow: /index.php?id=search\n") +
            TString("Disallow: /search/\n") +
            TString("Disallow: /edu/search/\n") +
            TString("Disallow: /edu/about/news/microsoft/\n") +
            TString("Disallow: /edu/order/\n") +
            TString("Disallow: /edu/certification/*/index.php\n") +
            TString("Disallow: /edu/study/cisco/index.php$\n") +
            TString("Disallow: /edu/study/cisco/$\n") +
            TString("Disallow: /edu/study/*/index.php?c=1\n") +
            TString("Disallow: /edu/study/microsoft/$\n") +
            TString("Disallow: /edu/study/microsoft/index.php$\n") +
            TString("Crawl-delay: 3\n") +
            TString("Clean-param: PHPSESSID *\n") +
            TString("Host: www.eureca.ru\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /cgi-bin/\n") +
            TString("Disallow: /edu/admin/\r\n") +
            TString("Disallow: /edu/admin_/\r") +
            TString("Disallow: /edu/admin__/\n") +
            TString("Disallow: /edu/admint/\n") +
            TString("Disallow: /admin/\r") +
            TString("Disallow: /transport/\n") +
            TString("Disallow: /index.php?id=search\r\n") +
            TString("Disallow: /search/\n") +
            TString("Disallow: /edu/search/\n") +
            TString("Disallow: /edu/about/news/microsoft/\n") +
            TString("Disallow: /edu/order/\r") +
            TString("Disallow: /edu/study/microsoft/$\r") +
            TString("Disallow: /edu/study/microsoft/index.php$\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        TVector<int> acceptedLines = robots.GetAcceptedLines();
        UNIT_ASSERT_EQUAL(acceptedLines.size(), 22);
        for (int i = 0; i < 22; ++i)
            UNIT_ASSERT_EQUAL(acceptedLines[i], i + 1);
    }

    inline void TestRobotsBazil10() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Disallow: /search\n") +
            TString("Disallow: /bin");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        TVector<int> acceptedLines = robots.GetAcceptedLines();

        UNIT_ASSERT_EQUAL(acceptedLines.size(), 3);
        for (int i = 0; i < 3; ++i)
            UNIT_ASSERT_EQUAL(acceptedLines[i], i + 1);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/searchxxx");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/search");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/bin");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/bon");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    inline void TestRobotsBazil11() {
        TString s =
            TString("User-agent: YandexBot/3.0\n") +
            TString("Disallow: /administrator/\n") +
            TString("Disallow: /cache\n") +
            TString("Disallow: /*print=1*\n") +
            TString("Disallow: /\n") +
            TString("Allow: /images/stories/my_images/\n") +
            TString("Allow: //stories/my_images/\n") +
            TString("Sitemap: https://ololo.com/sitemap.xml\n") +
            TString("Allow: /");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2;
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);

        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 1);
        UNIT_ASSERT_EQUAL(robots2.GetSiteMaps().size(), 1);
    }

    inline void TestRobotsBazil12() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Disallow: *//\n") +
            TString("Disallow: /*//\n") +
            TString("Disallow: *//$");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2;
        robots2.LoadPacked(robotsPacked);

        bool compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/banki//");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/banki//");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    // empty Allow directives are skipped now (https://st.yandex-team.ru/UKROP-795)
    inline void TestRobotsBazil13() {
        TString s =
            TString("User-agent: *\n") +
            TString("Allow: \n") +
            TString("Disallow: /search/\n") +
            TString("Disallow: /panel/");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        bool compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/search/vasya");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/panel/yarik");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/something/quick");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    // allow relative sitemap directives
    inline void TestRobotsBazil14() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /search/\n") +
            TString("\n") +
            TString("Sitemap: /sitemap.xml");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots, "http://stackoverflow.com");

        UNIT_ASSERT_EQUAL(robots.GetSiteMaps().size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetSiteMaps()[0], "http://stackoverflow.com/sitemap.xml");
    }

    inline void TestRobotsBazil15() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /yaca\n") +
            TString("Allow: /yaca/cat/*/$");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TRobotsTxt robots;
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/yaca/cat/Employment/Jobs/geo/");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/yaca/cat/");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/yaca/cat/xx");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/yaca/cat/xx/");
        UNIT_ASSERT_EQUAL(compare, false);
    }

    inline void TestRobotsPanda22() {
        TString s =
            TString("User-Agent: *\n") +
            TString("Disallow: /shop1/\n") +
            TString("Disallow: /shop2:a@xxx;ru\n") +
            TString("Crawl-delay: 6\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TRobotsTxt robots;
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2;
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop1/ ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop1/ ");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2:a@xxx");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2:a@xxx");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2:a@xxx;ru");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/shop2:a@xxx;ru");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsPanda23() {
        TString s =
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/?*\n") +
            TString("Disallow: /compare/\n") +
            TString("Disallow: /basket/\n") +
            TString("Disallow: /akcii/\n") +
            TString("Disallow: /katalog_tovarov/vip_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/ekonom_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/srednie_bukety/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/obychnye_bukety/\n") +
            TString("Disallow: /*/svadebnye_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_cvetu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_29_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/korporativ1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/bolee_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/s_pribavleniem/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/lublu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/skuhayu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosti/prosti/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_razmeru/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/serenada/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/ustoma/bukety_iz_eustom_i_drugih_cvetov/pautinka1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/belosnezhnyj_kupol/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/ekaterina/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz_i_drugih_cvetov/pushistik/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/gloriya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/podarok_iz_roz1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/klubnichnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/lesnoe_chudo/\n") +
            TString("Disallow: /katalog_tovarov/korziny_iz_cvetov1/korziny_iz_raznih_cvetov/korzina_s_liliyami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/dolores_s_kallami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/dolores_s_kallami1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/rozovye_rozy/buket_iz_21_rozovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/zheltye_rozy/serdce_iz_zheltyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zhemchuzhinka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/igrivyj/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/nezhnye_chuvstva/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/miks/raduga_iz_101_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem_i_drugih_cvetov/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/irisovaya_prelest/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/korzina_s_rozami_miks/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/ocharovanie/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/prichuda/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/frezii/bukety_iz_frezij_i_drugih_cvetov/belochka2/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gerbers/bukety_iz_gerber_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/krasnyj_kardinal/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/alla/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/bol_shaya_radost/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/skuchayu_po_tebe/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_orhidey/bravissimo2/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/prelest/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_hrizantemami/prelest1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/neobychnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_21_oranzhevoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/yubilej/konstanciya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/alenushka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_11_belyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/na_schast_e1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/kloun/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/lilies/bukety_iz_lilij/nezabyvaemyj_den2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_kremovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_eustomami/utrennyaya_roza1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zharkoe_leto/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zajchonok1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/populyarnye_bukety1/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem/oduvanchik/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/karameliya/\n") +
            TString("Disallow: /cart/\n") +
            TString("Disallow: /emarket/\n") +
            TString("Disallow: /katalog_tovarov/segodnya_dostavili/\n") +
            TString("Host: dailyflowers.ru\n") +
            TString("\n") +
            TString("User-Agent: Googlebot\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/?*\n") +
            TString("Disallow: /compare/\n") +
            TString("Disallow: /basket/\n") +
            TString("Disallow: /akcii/\n") +
            TString("Disallow: /katalog_tovarov/vip_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/ekonom_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/srednie_bukety/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/obychnye_bukety/\n") +
            TString("Disallow: /*/svadebnye_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_cvetu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_29_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/korporativ1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/bolee_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/s_pribavleniem/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/lublu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/skuhayu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosti/prosti/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_razmeru/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/serenada/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/ustoma/bukety_iz_eustom_i_drugih_cvetov/pautinka1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/belosnezhnyj_kupol/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/ekaterina/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz_i_drugih_cvetov/pushistik/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/gloriya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/podarok_iz_roz1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/klubnichnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/lesnoe_chudo/\n") +
            TString("Disallow: /katalog_tovarov/korziny_iz_cvetov1/korziny_iz_raznih_cvetov/korzina_s_liliyami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/dolores_s_kallami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/dolores_s_kallami1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/rozovye_rozy/buket_iz_21_rozovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/zheltye_rozy/serdce_iz_zheltyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zhemchuzhinka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/igrivyj/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/nezhnye_chuvstva/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/miks/raduga_iz_101_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem_i_drugih_cvetov/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/irisovaya_prelest/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/korzina_s_rozami_miks/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/ocharovanie/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/prichuda/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/frezii/bukety_iz_frezij_i_drugih_cvetov/belochka2/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gerbers/bukety_iz_gerber_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/krasnyj_kardinal/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/alla/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/bol_shaya_radost/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/skuchayu_po_tebe/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_orhidey/bravissimo2/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/prelest/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_hrizantemami/prelest1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/neobychnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_21_oranzhevoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/yubilej/konstanciya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/alenushka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_11_belyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/na_schast_e1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/kloun/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/lilies/bukety_iz_lilij/nezabyvaemyj_den2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_kremovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_eustomami/utrennyaya_roza1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zharkoe_leto/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zajchonok1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/populyarnye_bukety1/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem/oduvanchik/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/karameliya/\n") +
            TString("Disallow: /cart/\n") +
            TString("Disallow: /emarket/\n") +
            TString("Disallow: /katalog_tovarov/segodnya_dostavili/\n") +
            TString("\n") +
            TString("User-Agent: *\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/?*\n") +
            TString("Disallow: /compare/\n") +
            TString("Disallow: /basket/\n") +
            TString("Disallow: /akcii/\n") +
            TString("Disallow: /katalog_tovarov/vip_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/ekonom_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/srednie_bukety/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/obychnye_bukety/\n") +
            TString("Disallow: /*/svadebnye_bukety2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_cvetu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_29_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/do_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/korporativ1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/bolee_55_cvetov/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/s_pribavleniem/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/lublu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/skuhayu/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosti/prosti/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_razmeru/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/serenada/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/ustoma/bukety_iz_eustom_i_drugih_cvetov/pautinka1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/vse_cvety/belosnezhnyj_kupol/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/ekaterina/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz_i_drugih_cvetov/pushistik/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/gloriya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/podarok_iz_roz1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/klubnichnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/lesnoe_chudo/\n") +
            TString("Disallow: /katalog_tovarov/korziny_iz_cvetov1/korziny_iz_raznih_cvetov/korzina_s_liliyami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/dolores_s_kallami/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/dolores_s_kallami1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/rozovye_rozy/buket_iz_21_rozovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/zheltye_rozy/serdce_iz_zheltyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zhemchuzhinka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/igrivyj/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/nezhnye_chuvstva/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/rozy/miks/raduga_iz_101_rozy/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem_i_drugih_cvetov/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/yantarnyj_buket/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/irisovaya_prelest/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/korzina_s_rozami_miks/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/prosto_tak/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/statnyj/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/ocharovanie/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/grafinya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/prichuda/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/frezii/bukety_iz_frezij_i_drugih_cvetov/belochka2/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gerbers/bukety_iz_gerber_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz_i_drugih_cvetov/lesnoe_serdce/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/krasnyj_kardinal/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/alla/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/bol_shaya_radost/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/irises/bukety_iz_irisov_i_drugih_cvetov/skuchayu_po_tebe/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_orhidey/bravissimo2/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/prelest/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_hrizantemami/prelest1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/neobychnyj/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_21_oranzhevoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_povodu/yubilej/konstanciya/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall/alenushka/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/buket_iz_11_belyh_roz/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/101_roza/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_tipu/serdce_iz_cvetov/serdce_iz_roz/serdce_iz_51_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_roz/na_schast_e1/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/kloun/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/lilies/bukety_iz_lilij/nezabyvaemyj_den2/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_kremovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_s_eustomami/utrennyaya_roza1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kustovye_rozy/bukety_iz_kustovyh_roz/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_kolichestvu_cvetov_rozy_tyulpany/51_roza/buket_iz_51_rozovoj_kustovoj_rozy/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zharkoe_leto/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/zajchonok1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/kally/bukety_iz_kall_i_drugih_cvetov/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/po_stoimosti/ekonom_bukety/belosnezhnaya_nezhnost/\n") +
            TString("Disallow: /katalog_tovarov/podbor_buketa/populyarnye_bukety1/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/svadebnye_bukety/bukety_iz_kall/belosnezhnaya_nezhnost1/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/hrizantemy/bukety_iz_hrizantem/oduvanchik/\n") +
            TString("Disallow: /katalog_tovarov/katalog_cvetov/gvozdiki/bukety_iz_gvozdik/karameliya/\n") +
            TString("Disallow: /cart/\n") +
            TString("Disallow: /emarket/\n") +
            TString("Disallow: /katalog_tovarov/segodnya_dostavili/\n") +
            TString("\n") +
            TString("Sitemap: http://dailyflowers.ru/sitemap.xml\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/kustovye_rozy/");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/kustovye_rozy/");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/?param=xxx");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/katalog_tovarov/katalog_cvetov/?param=xxx");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsPanda24() {
        TString s =
            TString("User-agent: YandexBot\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsAllowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsAllowAll(robotstxtcfg::id_yandeximagesbot);
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsAllowAll(robotstxtcfg::id_yandeximagesbot);
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallowAll(robotstxtcfg::id_yandeximagesbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallowAll(robotstxtcfg::id_yandeximagesbot);
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsPanda25() {
        TString s =
            TString("User-agent: YandexCatalog\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Disallow: \n") +
            TString("\n") +
            TString("User-agent: YaDirectFetcher\n") +
            TString("Disallow: \n") +
            TString("\n") +
            TString("User-agent: YandexBot\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        // YandexCatalog - allow all,  YandexDirect - allow all, YandexBot - disallow all
        bool compare;
        compare = robots.IsAllowAll(robotstxtcfg::id_yandexcatalogbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllowAll(robotstxtcfg::id_yandexcatalogbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallowAll(robotstxtcfg::id_yandexcatalogbot);
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallowAll(robotstxtcfg::id_yandexcatalogbot);
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsAllowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsAllowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots.IsDisallowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsAllowAll(robotstxtcfg::id_yandexdirectbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllowAll(robotstxtcfg::id_yandexdirectbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot);
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot);
        UNIT_ASSERT_EQUAL(compare, false);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);

        UNIT_ASSERT_EQUAL(robots.IsAllowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsAllowAll(), false);

        robots.DoDisallowAll();
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), true);
        UNIT_ASSERT_EQUAL(robots.IsAllowAll(), false);

        robotsPackedSize = robots.GetPacked(robotsPacked);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), true);
        UNIT_ASSERT_EQUAL(robots2.IsAllowAll(), false);

        robots.DoAllowAll();
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots.IsAllowAll(), true);

        robotsPackedSize = robots.GetPacked(robotsPacked);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), false);
        UNIT_ASSERT_EQUAL(robots2.IsAllowAll(), true);

        robots.DoDisallowAll();
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(), true);
        UNIT_ASSERT_EQUAL(robots.IsAllowAll(), false);

        robotsPackedSize = robots.GetPacked(robotsPacked);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(), true);
        UNIT_ASSERT_EQUAL(robots2.IsAllowAll(), false);
    }

    inline void TestRobotsBazil7() {
        ui32 numOfRobots = 1000000;
        TVector<TSimpleSharedPtr<TRobotsTxt>> robotsArray(numOfRobots);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yandexblogsbot);
        botIds.insert(robotstxtcfg::id_yandexnewsbot);
        botIds.insert(robotstxtcfg::id_yandexpagechk);
        for (ui32 test = 0; test < numOfRobots; ++test) {
            if (test % 10000 == 0)
                Cout << test << Endl;

            TString s =
                TString("User-Agent: *\n") +
                TString("Disallow: /shop/\n") +
                TString("Crawl-delay: 6\n") +
                TString("\n") +
                TString("User-Agent: YaDirectFetcher\n") +
                TString("Disallow: /shop/\n") +
                TString("Crawl-delay: 2\n") +
                TString("\n") +
                TString("User-Agent: Google\n") +
                TString("Disallow: /shop/\n") +
                TString("Crawl-delay: 3\n") +
                TString("\n") +
                TString("User-Agent: Yandex\n") +
                TString("Disallow: /shop/\n") +
                TString("Disallow: /*lang=eng\n") +
                TString("Disallow: /*order_by\n") +
                TString("Disallow: /statistics/\n") +
                TString("Disallow: /info/\n") +
                TString("Disallow: /test/\n") +
                TString("Disallow: /partners/\n") +
                TString("Disallow: /albums/\n") +
                TString("Disallow: /groups/\n") +
                TString("Disallow: /hobby/\n") +
                TString("Disallow: /user/\n") +
                TString("Disallow: /profile/\n") +
                TString("Disallow: /reklama/\n") +
                TString("Disallow: /includes/\n") +
                TString("Disallow: /vacancy/search/\n") +
                TString("Disallow: /vacancy/friend_vacancy.html\n") +
                TString("Disallow: /vacancy/archive/\n") +
                TString("Disallow: /resume/search_resume.html\n") +
                TString("Disallow: /resume/search_region.html\n") +
                TString("Disallow: /resume/friend_resume.html\n") +
                TString("Disallow: /resume/send_resume.html\n") +
                TString("Disallow: /resume/views.html\n") +
                TString("Crawl-delay: 9\n") +
                TString("\n");

            TStringInput input(s);
            TRobotsTxtParser parser(input);

            robotsArray[test].Reset(new TRobotsTxt(botIds));
            TRobotsTxt::ParseRules(parser, &(*robotsArray[test]), &(*robotsArray[test]));
        }
    }

    inline void TestRobotsMemory() {
        TestRobotsMemoryImplementation(false);
        TestRobotsMemoryImplementation(true);
    }

    inline void TestRobotsMemoryImplementation(bool leak) {
        ui32 numOfRobots = 1000000;

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_yandexblogsbot);
        botIds.insert(robotstxtcfg::id_yandexnewsbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yandexpagechk);
        TVector<TSimpleSharedPtr<TRobotsTxt>> robotsArray(numOfRobots);
        TSimpleSharedPtr<TRobotsTxt> robots;

        //time_t start = 0;//time(0);

        for (ui32 i = 0; i < numOfRobots; ++i) {
            if (i % 20000 == 0 && i) {
                time_t late = 1; //time(0) - start;
                int res = late * numOfRobots / (i + 1);
                res -= late;
                Cout << "Now - " << late / 60 << ":" << late % 60 << "\t";
                Cout << "End - " << res / 60 << ":" << res % 60 << Endl;
            }
            TString s =
                TString("User-Agent: *\n") +
                TString("Disallow: /shop/\n") +
                TString("Crawl-delay: 6\n") +
                TString("\n") +
                TString("User-Agent: YaDirectFetcher \n") +
                TString("Disallow: /shop/\n") +
                TString("Crawl-delay: 2\n") +
                TString("\n") +
                TString("User-Agent: Google\n") +
                TString("Disallow: /shop/\n") +
                TString("Crawl-delay: 3\n") +
                TString("\n") +
                TString("User-Agent: Yandex\n") +
                TString("Disallow: /shop/\n") +
                TString("Disallow: /*lang=eng\n") +
                TString("Disallow: /*order_by\n") +
                TString("Disallow: /statistics/\n") +
                TString("Disallow: /info/\n") +
                TString("Disallow: /test/\n") +
                TString("Disallow: /partners/\n") +
                TString("Disallow: /albums/\n") +
                TString("Disallow: /groups/\n") +
                TString("Disallow: /hobby/\n") +
                TString("Disallow: /user/\n") +
                TString("Disallow: /profile/\n") +
                TString("Disallow: /reklama/\n") +
                TString("Disallow: /includes/\n") +
                TString("Disallow: /vacancy/search/\n") +
                TString("Disallow: /vacancy/friend_vacancy.html\n") +
                TString("Disallow: /vacancy/archive/\n") +
                TString("Disallow: /resume/search_resume.html\n") +
                TString("Disallow: /resume/search_region.html\n") +
                TString("Disallow: /resume/friend_resume.html\n") +
                TString("Disallow: /resume/send_resume.html\n") +
                TString("Disallow: /resume/views.html\n") +
                TString("Crawl-delay: 9\n") +
                TString("Host: finance.blogrange.com\n") +
                TString("\n") +
                TString("Sitemap: http://finance.blogrange.com/sitemap.xml.gz\n") +
                TString("Sitemap: http://finance.blogrange.com/sitemap2.xml.gz\n") +
                TString("\n");

            TStringInput input(s);
            TRobotsTxtParser parser(input);

            if (leak) {
                robots.Reset(new TRobotsTxt(botIds));
                TRobotsTxt::ParseRules(parser, robots.Get(), robots.Get());
            } else {
                robotsArray[i].Reset(new TRobotsTxt(botIds));
                TRobotsTxt::ParseRules(parser, &(*robotsArray[i]), &(*robotsArray[i]));
            }
        }
    }

    inline void TestRobotsPanda26() {
        TString s =
            TString("User-Agent: Yandex\n") +
            TString("Disallow: /$\n") +
            TString("Allow: /install/install\n") +
            TString("Disallow: /install/$\n") +
            TString("Disallow: /task/\n") +
            TString("Disallow: /links/\n") +
            TString("Disallow: /links.html\n") +
            TString("Host: www.artvolkov.ru\n") +
            TString("\n") +
            TString("User-Agent: *\n") +
            TString("Disallow: /install/\n") +
            TString("Disallow: /task/\n") +
            TString("Disallow: /links/\n") +
            TString("Disallow: /links.html\n") +
            TString("Host: www.artvolkov.ru\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsAllow(robotstxtcfg::id_anybot, "/");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllow(robotstxtcfg::id_anybot, "/");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/index.html");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/index.html");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsAllow(robotstxtcfg::id_yandexbot, "/install/install");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllow(robotstxtcfg::id_yandexbot, "/install/install");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/install/");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/install/");
        UNIT_ASSERT_EQUAL(compare, true);
    }

    inline void TestRobotsPanda27() {
        TString s =
            TString("User-Agent: Yandex\n") +
            TString("Allow: /$\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-Agent: *\n") +
            TString("Disallow: /install/\n") +
            TString("Disallow: /task/\n") +
            TString("Disallow: /links/\n") +
            TString("Disallow: /links.html\n") +
            TString("Host: www.artvolkov.ru\n") +
            TString("Clean-Param: ref /catalog/%D0%B0%D0%BA%D1%86%D0%B8%D0%B8\n") +
            TString("Clean-Param: comment\n") +
            TString("Clean-Param: from&place\n") +
            TString("Clean-Param: param1%5B%5D&param2[]&param3{}&param4{}&param5()&param6:123 /some_path\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        botIds.insert(robotstxtcfg::id_yandexcatalogbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;

        compare = robots.IsAllow(robotstxtcfg::id_yandexbot, "/");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllow(robotstxtcfg::id_yandexbot, "/");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/");
        UNIT_ASSERT_EQUAL(compare, false);

        compare = robots.IsAllow(robotstxtcfg::id_anybot, "/");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsAllow(robotstxtcfg::id_anybot, "/");
        UNIT_ASSERT_EQUAL(compare, true);

        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/index.html");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/index.html");
        UNIT_ASSERT_EQUAL(compare, true);

        TVector<TString> cleanParams = robots.GetCleanParams();
        TVector<TString> cleanParams2 = robots2.GetCleanParams();

        UNIT_ASSERT_EQUAL(cleanParams.size(), 4);
        UNIT_ASSERT_EQUAL(cleanParams2.size(), 4);
        UNIT_ASSERT_EQUAL(cleanParams[0] == "comment .*", true);
        UNIT_ASSERT_EQUAL(cleanParams[1] == "from&place .*", true);
        UNIT_ASSERT_EQUAL(cleanParams[2] == "param1%5B%5D&param2[]&param3{}&param4{}&param5()&param6:123 /some_path.*", true);
        UNIT_ASSERT_EQUAL(cleanParams2[2] == "param1%5B%5D&param2[]&param3{}&param4{}&param5()&param6:123 /some_path.*", true);
        UNIT_ASSERT_EQUAL(cleanParams[3] == "ref /catalog/%D0%B0%D0%BA%D1%86%D0%B8%D0%B8.*", true);
        UNIT_ASSERT_EQUAL(cleanParams2[3] == "ref /catalog/%D0%B0%D0%BA%D1%86%D0%B8%D0%B8.*", true);
    }

    inline void TestRobotsPanda28() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Disallow: /phpimageeditor/\n") +
            TString("Disallow: /related.php\n") +
            TString("Crawl-delay: 30\n") +
            TString("\n") +
            TString("User-agent: YandexBot/3.0\n") +
            TString("Disallow: /phpimageeditor/\n") +
            TString("Disallow: /related.php\n") +
            TString("Crawl-delay: 30\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("crawl-delay: 2\n") +
            TString("Disallow: /cgi-bin/\n") +
            TString("Disallow: /contest/\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);
        botIds.insert(robotstxtcfg::id_yandexnewsbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(), 30000);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), 30000);

        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 30000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 30000);
    }

    inline void TestRobotsQuick1() {
        TString s =
            TString("User-agent: *") + TString("\n") +
            TString("Disallow: /obsolete/private/") + TString("\n") +
            TString("Allow: /obsolete/private/*.html$") + TString("\n") +
            TString("Disallow: /*.php$") + TString("\n") +
            TString("Allow: /blabla/blabla/") + TString("\n") +
            TString("Disallow: /test1") + TString("\n") +
            TString("Allow: /test1.html") + TString("\n") +
            TString("Allow: /test2.html") + TString("\n") +
            TString("Disallow: /test2") + TString("\n") +
            TString("Disallow: /member1") + TString("\n") +
            TString("Allow: /member1") + TString("\n") +
            TString("Allow: /member2") + TString("\n") +
            TString("Disallow: /member2") + TString("\n") +
            TString("Allow: /test2.html") + TString("\n") +
            TString("Disallow: /1/private/") + TString("\n") +
            TString("Allow: /*/private/") + TString("\n") +
            TString("Disallow: /test2") + TString("\n") +
            TString("Disallow: /*/old/*.zip$") + TString("\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        TVector<TString> urls = {
            "/obsolete/private/smth.html",
            "/empt/bin.php",
            "/blabla/blabla/bin2.php",
            "/test1.html",
            "/test2.html",
            "/member1",
            "/member2",
            "/1/private/"};
        TVector<bool> answers = {
            true,
            false,
            true,
            true,
            true,
            true,
            true,
            false};
        UNIT_ASSERT_EQUAL(urls.size(), answers.size());
        CheckArray(urls, answers, robots);
        CheckArray(urls, answers, robots2);
    }

    inline void CheckArray(const TVector<TString>& urls, const TVector<bool>& answers, TRobotsTxt& robots) {
        for (size_t i = 0; i < urls.size(); ++i) {
            bool compare = robots.IsAllow(robotstxtcfg::id_anybot, urls[i].data());
            if (compare != answers[i]) {
                fprintf(stderr, "FAILL %s\n", urls[i].data());
            }
            UNIT_ASSERT_EQUAL(compare, answers[i]);
        }
    }

    inline void TestRobotsQuick2() {
        TString s =
            TString("# For domain: http://www.verdict.by/\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Disallow: /dir\n") +
            TString("\n") +
            TString("Disallow: /dir2\n") +
            TString("\n") +
            TString("Disallow: /dir3\n") +
            TString("\n") +
            TString("Crawl-delay: 2\n") +
            TString("Host: www.verdict.by\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        bool compare;
        compare = robots.IsDisallow(robotstxtcfg::id_anybot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, false);
        compare = robots2.IsDisallow(robotstxtcfg::id_anybot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_anybot), -1);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), -1);
        compare = robots.IsDisallow(robotstxtcfg::id_yandexbot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/dir3 ");
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 2000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 2000);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "www.verdict.by");
    }

    inline void TestRobotsQuick3() {
        TString s =
            TString("User-agent: *\n") +
            TString("\n") +
            TString("Disallow: /\n") +
            TString("Sitemap: https://google.ru/sitemap.xml\n") +
            TString("Host: https://google.ru/\n") +
            TString("\n");

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandexbotmirr);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        TVector<TString> sitemaps = robots.GetSiteMaps();
        TVector<TString> sitemaps2 = robots.GetSiteMaps();

        bool compare;
        compare = robots.IsDisallowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, true);
        compare = robots2.IsDisallowAll(robotstxtcfg::id_yandexbot);
        UNIT_ASSERT_EQUAL(compare, true);
        UNIT_ASSERT_EQUAL(sitemaps, sitemaps2);
        UNIT_ASSERT_EQUAL(sitemaps.size(), 1);
        UNIT_ASSERT_EQUAL(sitemaps[0], "https://google.ru/sitemap.xml");
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "https://google.ru/");
    }

    inline void TestRobotsQuick4() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /\n\n") +
            TString("User-agent: Google\n") +
            TString("Allow: /\n\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), true);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_googlebot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_googlebot), false);
    }

    inline void TestRobotsQuick5() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Host: something.ru/\n\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
    }

    inline void TestRobotsQuick6() {
        TString s =
            TString("User-agent: *\n") +
            TString("\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n") +
            TString("Host: yandex.ru\n") +
            TString("Crawl-delay: 10\n") +
            TString("Sitemap: http://google.ru\n") +
            TString("Clean-param: p\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);
        const TVector<TString> sitemaps = robots.GetSiteMaps();

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "yandex.ru");
        UNIT_ASSERT_EQUAL(sitemaps.size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), -1);
    }

    inline void TestRobotsQuick7() {
        TString s =
            TString("User-agent: *\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n") +
            TString("Host: yandex.ru\n") +
            TString("Crawl-delay: 10\n") +
            TString("Sitemap: http://google.ru\n") +
            TString("Clean-param: p\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);
        const TVector<TString> sitemaps = robots.GetSiteMaps();

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), true);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "yandex.ru");
        UNIT_ASSERT_EQUAL(sitemaps.size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 10000);
    }

    inline void TestRobotsQuick8() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Crawl-delay: 20\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n") +
            TString("Host: yandex.ru\n") +
            TString("Crawl-delay: 10\n") +
            TString("Sitemap: http://google.ru\n") +
            TString("Clean-param: p\n") +
            TString("\n") +
            TString("User-agent: Google\n") +
            TString("Crawl-delay: 10\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);
        const TVector<TString> sitemaps = robots.GetSiteMaps();

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "yandex.ru");
        UNIT_ASSERT_EQUAL(sitemaps.size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 20000);
    }

    inline void TestRobotsQuick9() {
        TString s =
            TString("User-agent: *\n") +
            TString("Crawl-delay: 10\n") +
            TString("\n") +
            TString("User-agent: StackRambler\n") +
            TString("Disallow: /\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);
        const TVector<TString> sitemaps = robots.GetSiteMaps();

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
    }

    inline void TestRobotsQuick10() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Host: yandex.ru\n") +
            TString("Crawl-delay: 10\n") +
            TString("Sitemap: http://google.ru\n") +
            TString("Clean-param: p\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);
        const TVector<TString> sitemaps = robots.GetSiteMaps();

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots.GetHostDirective(), "yandex.ru");
        UNIT_ASSERT_EQUAL(sitemaps.size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCleanParams().size(), 1);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 10000);
    }

    inline void TestRobotsQuick11() {
        TString s =
            TString("User-agent: Googlebot\n") +
            TString("User-agent: Yandex\n") +
            TString("User-agent: FeedFetcher\n") +
            TString("Crawl-delay: 2\n") +
            TString("Disallow: /sgw/\n") +
            TString("Disallow: /covers/\n") +
            TString("Disallow: /*checkval\n") +
            TString("Disallow: /*wicket:interface\n\n") +
            TString("# all others\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /\n\n") +
            TString("Sitemap: http://www.springer.com/googlesitemap\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
    }

    inline void TestRobotsQuick12() {
        TString s =
            TString("User-agent: Yandex\n") +
            TString("Host: www.yandex.ru\n\n") +

            TString("User-agent: *\n") +
            TString("Disallow: /\n\n") +
            TString("Sitemap: http://www.springer.com/googlesitemap\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
    }

    inline void TestRobotsQuick13() {
        TString s =
            TString("#\n") +
            TString("#\n") +
            TString("#  Hello World Web Hosting\n") +
            TString("# http://www.helloworldweb.com\n") +
            TString("# Removed old bots/spiders and added new 9-15-12, 5:35pm.\n") +
            TString("# Changed format 9-21-12 (put disallow immediately after User-agent string)\n") +
            TString("# to get stupid Yandex to fuck-the-hell-off. Yandex apparently goes out of it's way\n") +
            TString("# to NOT follow what's been a robots.txt standard for nearly a decade.\n") +
            TString("\n") +
            TString("Sitemap: http://helloworldweb.com/sitemap.xml\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: YandexBot/3.0\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Baiduspider\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: MJ12bot/v1.4.3\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: 008\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Aboundex/0.2\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Ezooms/1.0\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: CareerBot/1.1\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Comodo-Certificates-Spider\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("# added 9-26-12\n") +
            TString("User-agent: SeznamBot/3.0\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Disallow: /cgi-bin/\n") +
            TString("Disallow: /img/\n") +
            TString("Disallow: /billing/\n") +
            TString("Disallow: /portfolio/\n") +
            TString("Disallow: /images/\n") +
            TString("Disallow: /sandbox/\n") +
            TString("Disallow: /no_crawl/\n") +
            TString("Disallow: /js/\n") +
            TString("Disallow: /clients/\n") +
            TString("Disallow: /chat/\n") +
            TString("Disallow: /stats/\n") +
            TString("Disallow: /designcalendar/\n") +
            TString("Disallow: /clients/submitticket.php*\n") +
            TString("Disallow: /*.gif$\n") +
            TString("Disallow: /*.jpg$\n") +
            TString("Disallow: /*.jpeg$\n") +
            TString("Disallow: /*.png$\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), true);
    }

    inline void TestRobotsQuick14() {
        TString s =
            TString("# HTTP\n") +
            TString("\n") +
            TString("Sitemap: http://www.pedrelli.com/sitemapindex.xml\n") +
            TString("\n") +
            TString("User-agent: *\n") +
            TString("Crawl-delay: 5\n") +
            TString("Disallow: /en/nl/\n") +
            TString("Disallow: /it/nl/\n") +
            TString("Disallow: /nl/\n") +
            TString("\n") +
            TString("User-agent: AhrefsBot\n") +
            TString("User-agent: Baiduspider\n") +
            TString("User-agent: SeznamBot\n") +
            TString("User-agent: ShopWiki\n") +
            TString("User-agent: Sogou\n") +
            TString("User-agent: Spinn3r\n") +
            TString("User-agent: Yandex\n") +
            TString("User-agent: coccoc\n") +
            TString("User-agent: exabot\n") +
            TString("User-agent: ezooms\n") +
            TString("User-agent: fatbot\n") +
            TString("User-agent: jikespider\n") +
            TString("User-agent: psbot\n") +
            TString("User-agent: sosospider\n") +
            TString("User-agent: voilabot\n") +
            TString("User-agent: willybot\n") +
            TString("User-agent: yodaobot\n") +
            TString("Disallow: /\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), true);
    }

    inline void TestRobotsQuick15() {
        TString s =
            TString("User-agent: Slurp\n") +
            TString("Crawl-delay: 8\n") +
            TString("\n") +
            TString("User-agent: Msnbot\n") +
            TString("Crawl-delay: 8\n") +
            TString("\n") +
            TString("User-agent: YandexBot \n") +
            TString("Crawl-delay: 16\n") +
            TString("\n") +
            TString("# Some bots are known to be trouble, particularly those designed to copy\n") +
            TString("# entire sites. Please obey robots.txt.\n") +
            TString("\n") +
            TString("User-agent: MJ12bot \n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: dotbot \n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: spbot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: search17\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Speedy \n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: twiceler\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Exabot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: 008\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: SiteBot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: sitecheck.internetseer.com\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Zealbot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: MSIECrawler\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: SiteSnagger\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: WebStripper\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: WebCopier\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Fetch\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Offline Explorer\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Teleport\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: TeleportPro\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: WebZIP\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: linko\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: HTTrack\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Microsoft.URL.Control\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Xenu\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: larbin\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: libwww\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: ZyBORG\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Download Ninja\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: NPBot\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: WebReaper\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("# Crawlers that are kind enough to obey, but which we'd rather not have\n") +
            TString("# unless they're feeding search engines.\n") +
            TString("User-agent: UbiCrawler\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: DOC\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: Zao\n") +
            TString("Disallow: /\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexbot), false);
        UNIT_ASSERT_EQUAL(robots.GetCrawlDelay(robotstxtcfg::id_yandexbot), 16000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 16000);
    }

    inline void TestRobotsQuick16() {
        TString s =
            TString("User-agent: *\n") +
            TString("Crawl-delay: 10\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: YandexImages\n") +
            TString("Crawl-delay: 1\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(2000), 1000);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(2000), 1000);
    }

    inline void TestRobotsQuick17() {
        TString s =
            TString("User-agent: *\n") +
            TString("Crawl-delay: 3\n") +
            TString("\n") +
            TString("User-agent: Googlebot\n") +
            TString("Allow: /\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(2000), 3000);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(2000), 3000);
    }

    inline void TestRobotsQuick18() {
        TString s =
            TString("User-agent: *\n") +
            TString("Crawl-delay: 10\n") +
            TString("\n") +
            TString("User-agent: Yandex\n") +
            TString("Allow: /\n") +
            TString("\n") +
            TString("User-agent: YandexImages\n") +
            TString("Crawl-delay: 5\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.GetMinCrawlDelay(2000), -1);
        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(2000), -1);
    }

    inline void TestRobotsQuick19() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /\n") +
            TString("\n") +
            TString("User-agent: YandexImages\n") +
            TString("Disallow: /path\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path", false) != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandeximagesbot), false);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandeximagesbot, false), false);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path", false) != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandeximagesbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandeximagesbot, false), false);
    }

    inline void TestRobotsQuick20() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandeximagesbot), true);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandeximagesbot, false), false);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/path", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandeximagesbot, "/otherpath", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandeximagesbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandeximagesbot, false), false);
    }

    inline void TestRobotsQuick21() {
        TString s =
            TString("User-agent: *\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), true);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), false);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), false);
    }

    inline void TestRobotsQuick22() {
        TString s =
            TString("User-agent: YandexBot\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), false);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), false);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), false);
    }

    inline void TestRobotsQuick23() {
        TString s =
            TString("User-agent: YaDirectFetcher\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), true);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), true);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), true);
    }

    inline void TestRobotsQuick24() {
        TString s =
            TString("User-agent: YaDirectFetcher\n") +
            TString("Disallow: /\n") +
            TString("\n");

        TStringInput input(s);

        TRobotsTxtParser parser(input);
        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexdirectbot);

        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, true);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), true);
        UNIT_ASSERT_EQUAL(robots.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), true);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexdirectbot, "/", false) != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot), true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallowAll(robotstxtcfg::id_yandexdirectbot, false), true);
    }

    inline void TestRobotsCrawlDelayForDisallowAll() {
        TString s =
            "User-agent: *\n"
            "Disallow: /help.php\n"
            "Crawl-delay: 1\n"
            "User-agent: Google\n"
            "Disallow: /\n"
            "Crawl-delay: 42\n"
            "User-agent: YandexImages\n"
            "Disallow: /\n";

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), 1000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 1000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), 1000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), 42000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandeximagesbot), -1);
    }

    inline void TestRobotsCrawlDelayForDisallowAll2() {
        TString s =
            "User-agent: *\n"
            "Disallow: /help.php\n"
            "Crawl-delay: 42\n"
            "User-agent: Google\n"
            "Disallow: /\n"
            "Crawl-delay: 40\n"
            "User-agent: Yandex\n"
            "Disallow: /\n"
            "Crawl-delay: 10\n"
            "User-agent: YandexImages\n"
            "Disallow: /loging.php\n"
            "Crawl-delay: 20\n";

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_yandeximagesbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots2.GetMinCrawlDelay(), 20000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandexbot), 10000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_anybot), 42000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_googlebot), 40000);
        UNIT_ASSERT_EQUAL(robots2.GetCrawlDelay(robotstxtcfg::id_yandeximagesbot), 20000);
    }

    inline void TestRobotsMultipleBlocks() {
        TString s =
            "User-agent: *\n"
            "Disallow: /test\n"
            "User-agent: Yandex\n"
            "Disallow: /page\n"
            "User-agent: Yandex\n"
            "Disallow: /tags\n"
            "User-agent: *\n"
            "Disallow: /foo\n"
            "User-agent: Google\n"
            "Disallow: /googlepage\n"
            "User-agent: Google\n"
            "Disallow: /googletags\n";

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_yandexbot);
        botIds.insert(robotstxtcfg::id_googlebot);
        TRobotsTxt robots(botIds);
        robots.SetErrorsHandling(true);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        const char* robotsPacked = nullptr;
        size_t robotsPackedSize = robots.GetPacked(robotsPacked);
        Y_UNUSED(robotsPackedSize);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/page/123") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/tags/345") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/test/678") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/foo/910") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/people/345") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/googlepage/345") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_yandexbot, "/googletags/345") != nullptr, false);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/page/123") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/tags/345") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/test/678") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/foo/910") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/people/345") != nullptr, false);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/googlepage/345") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_googlebot, "/googletags/345") != nullptr, true);

        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_anybot, "/test/678") != nullptr, true);
        UNIT_ASSERT_EQUAL(robots2.IsDisallow(robotstxtcfg::id_anybot, "/foo/910") != nullptr, true);
    }

    inline void TestRobotsBOM() {
        TString s =
            "\xEF\xBB\xBF" // UTF-8 BOM
            "User-agent: *\n"
            "Disallow: /obsolete/private/\n"
            "Allow: /obsolete/private/*.html$\n"
            "Disallow: /*.php$\n"
            "Allow: /blabla/blabla/\n"
            "Disallow: /test1\n"
            "Allow: /test1.html\n"
            "Allow: /test2.html\n"
            "Disallow: /test2\n"
            "Disallow: /member1\n"
            "Allow: /member1\n"
            "Allow: /member2\n"
            "Disallow: /member2\n"
            "Allow: /test2.html\n"
            "Disallow: /1/private/\n"
            "Allow: /*/private/\n"
            "Disallow: /test2\n"
            "Disallow: /*/old/*.zip$\n"
            "\n";

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds;
        botIds.insert(robotstxtcfg::id_anybot);
        botIds.insert(robotstxtcfg::id_yandexbot);

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);
        const char* robotsPacked = nullptr;
        robots.GetPacked(robotsPacked);
        TRobotsTxt robots2(botIds);
        robots2.LoadPacked(robotsPacked);

        TVector<TString> urls = {
            "/obsolete/private/smth.html",
            "/empt/bin.php",
            "/blabla/blabla/bin2.php",
            "/test1.html",
            "/test2.html",
            "/member1",
            "/member2",
            "/1/private/"};
        TVector<bool> answers = {
            true,
            false,
            true,
            true,
            true,
            true,
            true,
            false};
        UNIT_ASSERT_EQUAL(urls.size(), answers.size());
        CheckArray(urls, answers, robots);
        CheckArray(urls, answers, robots2);
    }

    inline void TestRobotsRestore() {
        TString s =
            "User-Agent: *\n"
            "Disallow: /path1$\n"
            "Disallow: /path2*\n"
            "Disallow: /path3\n"
            "Disallow: /path4*path5*\n"
            "Disallow: /path6*path7$\n";

        TString expectedRestored =
            "User-Agent: *\n"
            "Disallow: /path1$\n"
            "Disallow: /path2\n" // expected to skip redundant '*'
            "Disallow: /path3\n"
            "Disallow: /path4*path5\n" // expected to skip redundant '*' at the end
            "Disallow: /path6*path7$\n";

        TStringInput input(s);
        TRobotsTxtParser parser(input);

        TBotIdSet botIds = {robotstxtcfg::id_anybot};

        TRobotsTxt robots(botIds);
        TRobotsTxt::ParseRules(parser, &robots, &robots);

        TString restored;
        TStringOutput out(restored);
        robots.Dump(robotstxtcfg::id_anybot, out);

        UNIT_ASSERT_EQUAL(restored, expectedRestored);
    }
};

UNIT_TEST_SUITE_REGISTRATION(TRobotsTxtTest);

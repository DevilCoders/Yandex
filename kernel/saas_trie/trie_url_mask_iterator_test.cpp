#include "trie_url_mask_iterator.h"

#include "disk_trie_test_helpers.h"

#include <kernel/saas_trie/idl/trie_key.h>

namespace NSaasTrie {
    namespace NTesting {
        TEST(TTrieComponentSuite, UrlMaskIterator) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetUrlMaskPrefix(".9");
            auto realm1 = key.AddAllRealms();
            realm1->SetName("9");
            realm1->AddKey("\thttp://www.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8");
            realm1->AddKey("\thttps://abcimagess.com/veins+popping+in+legs");
            realm1->AddKey("\thttp://something.com.tr/cool-story");
            realm1->AddKey("\ta.b.c/1/2.html?param=1");
            realm1->AddKey("\thttp://ya.ru/1/2/");
            realm1->AddKey("\thttp://awesome.site.co.uk/index.html");
            realm1->AddKey("\thttps://subdomain.example.com/folder/document");
            realm1->AddKey("\tstrange.url?");
            realm1->AddKey("\tstrange.ur?a=10");
            realm1->AddKey("\t.");
            realm1->AddKey("\t./");
            realm1->AddKey("\t.?");
            key.AddKeyRealms("9");

            TestComplexKeyIterator(
                context,
                {
                    {"0\t.9\twww.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", 10},
                    {"0\t.9\twww.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8/", 11},
                    {"0\t.9\twww.funnycat.tv/", 0},
                    {"0\t.9\twww.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/", 1},
                    {"0\t.9\tfunnycat.tv/video/", 2},
                    {"0\t.9\tabcimagess.com/veins+popping+in+legs/", 3},
                    {"0\t.9\tabcimagess.com/", 4},
                    {"0\t.9\tcom.tr/cool-story", 5},
                    {"0\t.9\tsomething.com.tr/cool-story", 6},
                    {"0\t.9\ta.b.c/1/2.html?param=1", 20},
                    {"0\t.9\ta.b.c/1/2.html", 21},
                    {"0\t.9\ta.b.c/", 22},
                    {"0\t.9\ta.b.c/1/", 23},
                    {"0\t.9\tb.c/1/2.html?param=1", 24},
                    {"0\t.9\tb.c/1/2.html", 25},
                    {"0\t.9\tb.c/", 26},
                    {"0\t.9\tb.c/1/", 27},
                    {"0\t.9\tya.ru/1/2/", 28},
                    {"0\t.9\tya.ru/", 29},
                    {"0\t.9\tya.ru/1/", 30},
                    {"0\t.9\tsubdomain.example.com/fold", 40},
                    {"0\t.9\tsubdomain.example.com/folder/doc", 41},
                    {"0\t.9\texample.com/folder/", 42},
                    {"0\t.9\tstrange.url/", 50},
                    {"0\t.9\tstrange.ur/", 51},
                    {"0\t.9\t./", 52},
                    {"0\t.9\tstrange.ur?a=10", 53}
                },
                key,
                {
                    {"0\t.9\twww.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", 10},
                    {"0\t.9\twww.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8/", 11},
                    {"0\t.9\twww.funnycat.tv/", 0},
                    {"0\t.9\twww.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/", 1},
                    {"0\t.9\tfunnycat.tv/video/", 2},
                    {"0\t.9\tabcimagess.com/veins+popping+in+legs/", 3},
                    {"0\t.9\tabcimagess.com/", 4},
                    {"0\t.9\tsomething.com.tr/cool-story", 6},
                    {"0\t.9\ta.b.c/1/2.html?param=1", 20},
                    {"0\t.9\ta.b.c/1/2.html", 21},
                    {"0\t.9\ta.b.c/", 22},
                    {"0\t.9\ta.b.c/1/", 23},
                    {"0\t.9\tb.c/1/2.html?param=1", 24},
                    {"0\t.9\tb.c/1/2.html", 25},
                    {"0\t.9\tb.c/", 26},
                    {"0\t.9\tb.c/1/", 27},
                    {"0\t.9\tya.ru/1/2/", 28},
                    {"0\t.9\tya.ru/", 29},
                    {"0\t.9\tya.ru/1/", 30},
                    {"0\t.9\texample.com/folder/", 42},
                    {"0\t.9\tstrange.url/", 50},
                    {"0\t.9\tstrange.ur/", 51},
                    {"0\t.9\t./", 52},
                    {"0\t.9\t./", 52},
                    {"0\t.9\t./", 52}
                }
            );
        }

        TEST(TTrieComponentSuite, MixedIterator) {
            TDiskTrieTestContext context;
            TComplexKey key;
            key.SetMainKey(".7");
            key.SetUrlMaskPrefix(".9");
            auto realm1 = key.AddAllRealms();
            realm1->SetName("7");
            realm1->AddKey("\thttp://example.com/somefolder/page.html?x=1");
            key.AddKeyRealms("7");

            TestComplexKeyIterator(
                context,
                {
                    {"0\t.7\thttp://example.com/somefolder/page.html?x=1", 37},
                    {"0\t.9\texample.com/somefolder/", 42}
                },
                key,
                {
                    {"0\t.7\thttp://example.com/somefolder/page.html?x=1", 37},
                    {"0\t.9\texample.com/somefolder/", 42}
                }
            );
        }

        void CheckHostIterator(TStringBuf url, const TVector<TStringBuf>& expectedHosts) {
            TUrlMaskHostIterator iterator;
            iterator.SetUrl(url);
            if (iterator.AtEnd() || !iterator.CheckOwner()) {
                EXPECT_EQ(0, expectedHosts.size());
            } else {
                size_t i = 0;
                do {
                    ASSERT_LT(i, expectedHosts.size());
                    EXPECT_EQ(iterator.CurrentHost(), expectedHosts[i]);
                    ++i;
                } while (iterator.Next() && iterator.CheckOwner());
                EXPECT_EQ(i, expectedHosts.size());
            }
        }

        TEST(TTrieComponentSuite, UrlMaskHostIterator) {
            CheckHostIterator("www.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", {
                "www.funnycat.tv",
                "funnycat.tv"
            });
            CheckHostIterator("protectload.ru/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html", {
                "protectload.ru"
            });
            CheckHostIterator("http://abcimagess.com/veins+popping+in+legs", {
                "abcimagess.com"
            });
            CheckHostIterator("http://a.b.c/1/2.html?param=1", {
                "a.b.c",
                "b.c"
            });
            CheckHostIterator("http://a.b.c.d.e.f.g/1.html", {
                "a.b.c.d.e.f.g",
                "c.d.e.f.g",
                "d.e.f.g",
                "e.f.g",
                "f.g"
            });
            CheckHostIterator("https://a.b.c.d.e.f.g", {
                "a.b.c.d.e.f.g",
                "c.d.e.f.g",
                "d.e.f.g",
                "e.f.g",
                "f.g"
            });
            CheckHostIterator("h/a/b/c/d/e/f/g/", {
                "h"
            });
            CheckHostIterator("f.g/a/", {
                "f.g"
            });
            CheckHostIterator("a.b?", {
                "a.b"
            });
            CheckHostIterator(".", {
                "."
            });
            CheckHostIterator("./", {
                "."
            });
            CheckHostIterator(".?", {
                "."
            });
        }

        void CheckPathIterator(TStringBuf url, const TVector<std::pair<TStringBuf, bool>>& expectedPaths) {
            TUrlMaskPathIterator iterator;
            iterator.SetUrl(url);
            if (iterator.AtEnd()) {
                EXPECT_EQ(0, expectedPaths.size());
            } else {
                size_t i = 0;
                do {
                    ASSERT_LT(i, expectedPaths.size());
                    EXPECT_EQ(iterator.CurrentPath(), expectedPaths[i].first);
                    EXPECT_EQ(iterator.EndsWithSlash(), expectedPaths[i].second);
                    ++i;
                } while (iterator.Next());
                EXPECT_EQ(i, expectedPaths.size());
            }
        }

        TEST(TTrieComponentSuite, UrlMaskPathIterator) {
            CheckPathIterator("www.funnycat.tv/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", {
                {"/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", false},
                {"/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/BV1nr_yFnf8", true},
                {"/", false},
                {"/video/", false},
                {"/video/avast-premier-2016-kljuch-licenzii-do-2021-2022/", false}
            });
            CheckPathIterator("protectload.ru/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html", {
                {"/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html", false},
                {"/Keys/klyuchi-dlya-avast-premier-kod-aktivacii-2016.html", true},
                {"/", false},
                {"/Keys/", false}
            });
            CheckPathIterator("http://abcimagess.com/veins+popping+in+legs", {
                {"/veins+popping+in+legs", false},
                {"/veins+popping+in+legs", true},
                {"/", false}
            });
            CheckPathIterator("http://a.b.c/1/2.html?param=1", {
                {"/1/2.html?param=1", false},
                {"/1/2.html", false},
                {"/", false},
                {"/1/", false}
            });
            CheckPathIterator("http://a.b.c.d.e.f.g/1.html", {
                {"/1.html", false},
                {"/1.html", true},
                {"/", false},
            });
            CheckPathIterator("https://a.b.c.d.e.f.g", {
                {"/", false}
            });
            CheckPathIterator("http://a.b.c.d.e.f.g/", {
                {"/", false}
            });
            CheckPathIterator("h/a/b/c/d/e/f/g/", {
                {"/a/b/c/d/e/f/g/", false},
                {"/", false},
                {"/a/", false},
                {"/a/b/", false},
                {"/a/b/c/", false},
                {"/a/b/c/d/", false}
            });
            CheckPathIterator("h/a/b/c/d/e/f/g", {
                {"/a/b/c/d/e/f/g", false},
                {"/a/b/c/d/e/f/g", true},
                {"/", false},
                {"/a/", false},
                {"/a/b/", false},
                {"/a/b/c/", false}
            });
            CheckPathIterator("c.d.e.f.g/a/b/c/d/", {
                {"/a/b/c/d/", false},
                {"/", false},
                {"/a/", false},
                {"/a/b/", false},
                {"/a/b/c/", false}
            });
            CheckPathIterator("e.f.g/a/b/", {
                {"/a/b/", false},
                {"/", false},
                {"/a/", false}
            });
            CheckPathIterator("f.g/a/", {
                {"/a/", false},
                {"/", false}
            });
            CheckPathIterator("a.b?", {
                {"/", false}
            });
            CheckPathIterator(".", {
                {"/", false}
            });
            CheckPathIterator("./", {
                {"/", false}
            });
            CheckPathIterator(".?", {
                {"/", false}
            });
        }
    }
}

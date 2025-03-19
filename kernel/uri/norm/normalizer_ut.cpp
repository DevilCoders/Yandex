#include <library/cpp/testing/unittest/registar.h>

#include "normalizer.h"

#define NORM_EQUAL(a, b) UNIT_ASSERT_VALUES_EQUAL(a, norm.NormalizeUrl(b))
#define NORM_UNEQUAL(a, b) UNIT_ASSERT_VALUES_UNEQUAL(a, norm.NormalizeUrl(b))

Y_UNIT_TEST_SUITE(TQualityUtilsHostNormalizerTest)
{
    Y_UNIT_TEST(testFailures)
    {
        Nydx::TUriNormalizer norm;
        NORM_EQUAL("http://vk.ru/", "http://vk.ru");
        NORM_EQUAL("http://www.flickr.com/photos/nyon", "www.flickr.com/photos/nyon?");
        NORM_EQUAL("http://a.b.c/foo/bar/?a=1&b=2", "a.b.c/foo/bar/index.html?b=2&a=1#frag");
        NORM_EQUAL("http://a.b.c/foo/bar/?a=1&b=2&b=3&b=a", "a.b.c/foo/bar/default.aspx?b=3;b=a;b=2;a=1");
        NORM_EQUAL("http://a.b.c/?=a", "a.b.c/index.html?=a");
        NORM_EQUAL("http://a.b.c/", "a.b.c/index.php");
        NORM_EQUAL("http://a.b.c/foo/bar/?=a", "a.b.c/foo/bar/default.aspx?=a");
        NORM_EQUAL("http://host:8080/?q", "host:8080?q");
        NORM_UNEQUAL("http://images.zone-x.ru/162/645482.jpg"
            , "images.zone-x.ru/162%5C645482.jpg");
        norm.DisableQrySort();
        NORM_EQUAL("http://a.b.c/foo/bar/?b=2&a=1", "a.b.c/foo/bar/index.html?b=2;a=1#frag");
        norm.DisableQryNormDelim();
        NORM_EQUAL("http://a.b.c/foo/bar/?b=3;b=a;b=2;a=1", "a.b.c/foo/bar/default.aspx?b=3;b=a;b=2;a=1");
        NORM_EQUAL("http://a.b.c/foo/bar/?b=3;b=a", "a.b.c/foo/bar/default.aspx?b=3;b=a;b=3");
        norm.DisableQryAltSemicol();
        NORM_EQUAL("http://a.b.c/foo/bar/?b=3;b=a;b=3", "a.b.c/foo/bar/default.aspx?b=3;b=a;b=3");
        NORM_EQUAL("http://a.b.c/foo/bar/?b=3&b=a", "a.b.c/foo/bar/default.aspx?b=3&b=a&b=3");
        norm.DisableDecodeHTML();
        norm.DisableNormalizeEscFrag();
        norm.EnableForcePathRemoveTrailSlash();
        NORM_EQUAL("http://a.b.c/", "a.b.c/");
        NORM_EQUAL("http://a.b.c/?=a", "a.b.c/index.html?=a");
        NORM_EQUAL("http://a.b.c/", "a.b.c/index.php");
        NORM_EQUAL("http://a.b.c/foo/bar?=a", "a.b.c/foo/bar/default.aspx?=a");
        norm.DisablePathRemoveIndexPage();
        norm.DisableQryRemoveDups();
        NORM_EQUAL("http://a.b.c/foo/bar/default.aspx?b=3&b=a&b=3", "A.b.C/foo/bar//default.aspx?b=3&b=a&b=3");
        norm.EnableForcePathAppendDirTrailSlash();
        NORM_EQUAL("http://a.b.c/default.aspx", "A.b.C///default.aspx");
        NORM_EQUAL("http://a.b.c/foo/", "A.b.C///foo");
    }

    Y_UNIT_TEST(testAmazon)
    {
        Nydx::TUriNormalizer norm;
        norm.EnableRemoveWWW();
        NORM_EQUAL("http://amazon.com/dp/BLAH", "www.amazon.com/FOOBAR/dp/BLAH/ref=marazm?qry=true");
        NORM_EQUAL("http://amazon.com/dp/BLAH", "www.amazon.com/FOOBAR/gp/product/BLAH");
        NORM_EQUAL("http://amazon.com/dp/BLAH", "www.amazon.com/FOOBAR/o/ASIN/BLAH");
        NORM_EQUAL("http://amazon.com/dp/BLAH", "www.amazon.com/FOOBAR/exec/obidos/tg/detail/-/BLAH");
        NORM_EQUAL("http://amazon.com/o/BLAH", "www.amazon.com/FOOBAR/o/BLAH");
        NORM_EQUAL("http://amazon.com/product-reviews/BLAH", "www.amazon.com/FOOBAR/product-reviews/BLAH");
    }

    Y_UNIT_TEST(testGoogle)
    {
        Nydx::TUriNormalizer norm;
        NORM_EQUAL("http://www.google.com/search?hl=1", "www.google.com/search?hl=1&a=1&b=2");
        norm.EnableRemoveWWW();
        NORM_EQUAL("http://google.com/search?hl=1&tbm=&tbo", "www.google.com/search?hl=1&a=1&tbo&tbm=");
        NORM_EQUAL("http://maps.google.com/BLAH?b=1", "maps.google.com/BLAH?ei=1&b=1");
        NORM_EQUAL("http://google.com/ig?=123&a=1&type=1", "http://google.com/ig?type=1;a=1;=123");
        NORM_EQUAL("http://b.com/BLAH?=2&b=1"
            , "http://www.google.com/url?a=1;url=http%3a%2f/www.b.com/BLAH?b=1%26=2");
        NORM_EQUAL("http://google.com/search?hl=en&q=marazm&source=hp"
            , "https://www.google.com/webhp#hl=en&site=webhp&source=hp&q=marazm&pbx=1&gs_sm=e");
    }

    Y_UNIT_TEST(testWiki)
    {
        Nydx::TUriNormalizer norm;
        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/Rob_Lowe"
            , "http://en.wikipedia.org/?title=Rob_Lowe");
        NORM_EQUAL(
            "http://en.wikipedia.org/w/index.php?search=Rob+Lowe"
            , "http://en.wikipedia.org/wiki/Search?search=Rob+Lowe&title=Special+Search");
        NORM_EQUAL(
            "http://en.wikipedia.org/w/index.php?search=Rob+Lowe"
            , "http://en.wikipedia.org/w/index.php?search=Rob+Lowe&title=Special+Search");
        // converts CP1251 to UTF8, hex escapes to uppercase
        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%A2%D0%B5%D0%BB%D0%B5%D0%BF%D0%BE%D1%80%D1%82_(%D1%84%D0%B8%D0%BB%D1%8C%D0%BC)"
            , "http://ru.wikipedia.org/wiki/%d2%e5%eb%e5%ef%ee%f0%f2_%28%f4%e8%eb%fc%ec%29");
        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%A2%D0%B5%D0%BB%D0%B5%D0%BF%D0%BE%D1%80%D1%82_(%D1%84%D0%B8%D0%BB%D1%8C%D0%BC)"
            , "ru.wikipedia.org/wiki/%d0%a2%d0%b5%d0%bb%d0%b5%d0%bf%d0%be%d1%80%d1%82_(%d1%84%d0%b8%d0%bb%d1%8c%d0%bc)");
        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%9F%D0%B5%D1%82%D1%82%D0%B8%D1%84%D0%B5%D1%80,_%D0%90%D0%BB%D0%B5%D0%BA%D1%81"
            , "ru.wikipedia.org/wiki/%cf%e5%f2%f2%e8%f4%e5%f0,_%c0%eb%e5%ea%f1");

        // underscores and spaces are equivalent // http://en.wikipedia.org/wiki/Help:URL#URLs_of_Wikipedia_pages // http://en.wikipedia.org/wiki/Wikipedia:Canonical#Conversion_to_canonical_form
        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C_%D0%98%D0%BE%D1%81%D0%B8%D1%84_%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87"
            , "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C%20%D0%98%D0%BE%D1%81%D0%B8%D1%84%20%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87");

        // spaces (underscores) at the start or at the end of query or namespace // http://en.wikipedia.org/wiki/Wikipedia:Canonical#Conversion_to_canonical_form
        // two spaces together are treated as one // http://en.wikipedia.org/wiki/Wikipedia:Canonical#Conversion_to_canonical_form
        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C_%D0%98%D0%BE%D1%81%D0%B8%D1%84_%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87"
            , "http://ru.wikipedia.org/wiki/_%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C%20%D0%98%D0%BE%D1%81%D0%B8%D1%84%20%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87");

        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C_%D0%98%D0%BE%D1%81%D0%B8%D1%84_%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87"
            , "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C%20%D0%98%D0%BE%D1%81%D0%B8%D1%84%20%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87_");

        NORM_EQUAL(
            "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C_%D0%98%D0%BE%D1%81%D0%B8%D1%84_%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87"
            , "http://ru.wikipedia.org/wiki/%D0%A1%D1%82%D0%B0%D0%BB%D0%B8%D0%BD%2C%20%D0%98%D0%BE%D1%81%D0%B8%D1%84%20%D0%92%D0%B8%D1%81%D1%81%D0%B0%D1%80%D0%B8%D0%BE%D0%BD%D0%BE%D0%B2%D0%B8%D1%87");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/User:Jimbo_Wales"
            , "http://en.wikipedia.org/wiki/_User_:_Jimbo___Wales__");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/User:Jimbo_Wales"
            , "http://en.wikipedia.org/wiki/_User:Jimbo_Wales");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/User:Jimbo_Wales"
            , "http://en.wikipedia.org/wiki/User_:Jimbo_Wales");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/User:Jimbo_Wales"
            , "http://en.wikipedia.org/wiki/User:_Jimbo_Wales");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/User:"
            , "http://en.wikipedia.org/wiki/User:_");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/User:"
            , "http://en.wikipedia.org/wiki/User:");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/:Jimbo_Wales"
            , "http://en.wikipedia.org/wiki/_:_Jimbo_Wales");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/:Jimbo_Wales"
            , "http://en.wikipedia.org/wiki/:Jimbo_Wales");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/"
            , "http://en.wikipedia.org/wiki/_");

        NORM_EQUAL(
            "http://en.wikipedia.org/wiki/"
            , "http://en.wikipedia.org/wiki/__");
    }

    Y_UNIT_TEST(testTwitter)
    {
        Nydx::TUriNormalizer norm;
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/#!a/b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/#!/a/b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a#b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a#/b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a/#b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a/#/b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a#!b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a#!/b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a/#!b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a/#!/b/c");
        NORM_EQUAL("http://twitter.com/a/b/c", "http://twitter.com/a/?_escaped_fragment_=/b/c");
        NORM_EQUAL("http://nottwitter.com/a/#!/b/c", "http://nottwitter.com/a/?_escaped_fragment_=/b/c");
        NORM_EQUAL("http://nottwitter.com/a/#!/b/c", "http://nottwitter.com/a/#!/b/c");
        norm.EnablePathRemoveTrailSlash();
        NORM_EQUAL("http://nottwitter.com/a#!/b/c", "http://nottwitter.com/a/?_escaped_fragment_=/b/c");
        NORM_EQUAL("http://nottwitter.com/a#!/b/c", "http://nottwitter.com/a/#!/b/c");
    }

    Y_UNIT_TEST(testPlusLower)
    {
        Nydx::TUriNormalizer norm;
        norm.EnableEncodeSpcAsPlus();
        norm.EnableLowercaseURL();
        NORM_EQUAL("http://www.host.com/?search=parasite%2Bcity"
            , "www.HOST.com/index.php?search=Parasite%2BCity");
        NORM_EQUAL("http://www.host.com/parasite+city/"
            , "www.HOST.com/Parasite%20City/index.php");
        norm.EnableEncodeExtended();
        norm.DisableLowercaseURL();
        NORM_EQUAL("http://www.host.com/%5Bsomepath=%27weird%27%5D/"
            , "www.HOST.com/[somepath='weird']/InDeX.php5");
        norm.EnableRemoveWWW();
        TString dst;
        UNIT_ASSERT(norm.NormalizeHost("WWW.HOST.COM", dst));
        UNIT_ASSERT_VALUES_EQUAL("host.com", dst);
    }

    Y_UNIT_TEST(testExtendedASCII)
    {
        Nydx::TUriNormalizer norm;
        NORM_UNEQUAL("http://xn--b1afab7bff7cb.xn--p1ai/", "Череповец。рф/");
        norm.EnableHostAllowIDN();
        NORM_EQUAL("http://xn--b1afab7bff7cb.xn--p1ai/", "Череповец。рф/");
    }
}

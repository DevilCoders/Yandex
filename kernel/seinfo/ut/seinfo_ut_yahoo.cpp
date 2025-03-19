#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc13() {
    // yahoo
    {
        TInfo info(SE_YAHOO, ST_WEB, "howto", SF_SEARCH);

        KS_TEST_URL("http://search.yahoo.com/search?p=howto", info);
        KS_TEST_URL("http://search.yahoo.com/search;_ylt=A0SO8Zt.DLdMdHwAFmz7w8QF;_ylu=X3oDMTBlazNvOWJmBHNlYwN0YWJzBHZ0aWQD?ei=UTF-8&p=howto&fr2=tab-video&fr=yfp-t-701", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://images.search.yahoo.com/search/images;_ylt=A0oG7mkZDLdMeiQAJCdXNyoA?ei=UTF-8&p=howto&fr2=tab-web&fr=yfp-t-701", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.search.yahoo.com/search/video;_ylt=A0oG7laADLdMGiIAioNXNyoA?ei=UTF-8&p=howto&fr2=tab-web&fr=yfp-t-701", info);

        info.Query = "школа";
        info.Type = ST_WEB;
        KS_TEST_URL("http://ru.search.yahoo.com/search;_ylt=A03uv8vVDLdMF9IAZw3Lxgt.?p=%D1%88%D0%BA%D0%BE%D0%BB%D0%B0&fr2=sb-top&fr=FP-tab-web-t", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://ru.images.search.yahoo.com/search/images;_ylt=A1f4cfrZDLdM674AWB_Lxgt.?ei=UTF-8&p=%D1%88%D0%BA%D0%BE%D0%BB%D0%B0&fr2=tab-web&fr=FP-tab-web-t", info);

        // Added by vvp@. Commented by smikler@. Not a search.
        //KS_TEST_URL("http://video.yahoo.com/search/?p=go&t=video", info);
        //KS_TEST_URL("http://images.yahoo.com/search", info);
        //KS_TEST_URL("http://www.yahoo.com/search", info);
        //KS_TEST_URL("http://search.yahoo.com/", info);

        info.Query = "12 34";
        info.Type = ST_WEB;
        KS_TEST_URL("http://ru.search.yahoo.com/search;_ylt=AtRyqVGdEZUdCxCLKJWgGfitk7x_;_ylc=X1MDMjE0MzA2NTAwNQRfcgMyBGZyA3lmcC10LTcyMgRuX2dwcwMxMARvcmlnaW4DcnUueWFob28uY29tBHF1ZXJ5AzEyIDM0BHNhbwMx?vc=&p=12+34&toggle=1&cop=mss&ei=UTF-8&fr=yfp-t-722", info);
        info.Type = ST_IMAGES;
        KS_TEST_URL("http://ru.images.search.yahoo.com/search/images;_ylt=A7x9QXu.w9hO9CgAsRbLxgt.?ei=UTF-8&p=12%2034&fr2=tab-web&fr=yfp-t-722", info);
        info.Type = ST_NEWS;
        KS_TEST_URL("http://ru.news.search.yahoo.com/search/news;_ylt=A0PDodnpw9hO2zIAuzzNxgt.?p=12+34&fr=&fr=yfp-t-722&fr2=tab-web", info);

        info.Query = "123";
        info.Type = ST_WEB;
        KS_TEST_URL("http://search.yahoo.com/search;_ylt=Ap_btlgKu.8LG6MhH3bZ7S.bvZx4?p=123&toggle=1&cop=mss&ei=UTF-8&fr=yfp-t-701", info);
        KS_TEST_URL("http://us.yhs4.search.yahoo.com/yhs/search;_ylt=A0oG7pvg3thOuW4AYgel87UF;_ylc=X1MDMjE0MjQ3ODk0OARfcgMyBGZyA2FsdGF2aXN0YQRuX2dwcwMwBG9yaWdpbgNzeWMEcXVlcnkDMTIzBHNhbwMw?p=123&fr=altavista&fr2=sfp&iscqry=", info);
        KS_TEST_URL("https://uk.search.yahoo.com/search?p=123&fr=yfp-t-403", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://images.search.yahoo.com/search/images;_ylt=A0oG7iAqxNhOgH4AQdpXNyoA?p=123&fr2=piv-web", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://video.search.yahoo.com/search/video;_ylt=A0PDoYA0xNhOpzwAueeJzbkF?p=123&fr=&fr2=piv-web", info);
        info.Type = ST_COM;
        KS_TEST_URL("http://shopping.yahoo.com/search;_ylt=A2KLqIVGxNhOoxIAOuT7w8QF?p=123&fr2=piv-video", info);
        KS_TEST_URL("http://finance.search.yahoo.com/search;_ylt=A2KJ3CZyxNhOtnEATvFNbK5_?&p=123&fr2=piv-sports&fr=", info);
        info.Type = ST_APPS;
        KS_TEST_URL("http://apps.search.yahoo.com/search?p=123&fr2=piv-video", info);
        info.Type = ST_BLOGS;
        KS_TEST_URL("http://blog.search.yahoo.com/search;_ylt=A0oG7lxgxNhOyngADHRV99l_?p=123&fr2=piv-web", info);
        info.Type = ST_SPORT;
        KS_TEST_URL("http://sports.search.yahoo.com/search;_ylt=A2KLOzFqxNhOjU8AuPyaxAt.?&p=123&fr2=piv-blog&fr=", info);
        info.Type = ST_RECIPES;
        KS_TEST_URL("http://recipes.search.yahoo.com/search?&p=123&fr2=piv-finance&fr=", info);

        info.Type = ST_WEB;
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://m.yahoo.com/w/search%3B_ylt=A2KL8wqT3dhOfzIAjh0p89w4?submit=oneSearch&.intl=gb&.lang=en-gb&.tsrc=yahoo&.sep=fp&p=123&x=0&y=0", info);
        KS_TEST_URL("http://m.yahoo.com/w/search%3B_ylt=A2KL8wS83dhOR3sA2xcp89w4?submit=oneSearch&.intl=RU&.lang=ru&.tsrc=yahoo&.sep=fp&p=123&x=26&y=11", info);

        info.Clear();
        // TODO: Safely can recognize yahoo in urls
        KS_TEST_URL("https://id.yahoo.com/?p=us", info);
        KS_TEST_URL("https://ads.yahoo.com/imp?K=1&Z=300x250&cb=1443597773.863743&x=https%3A%2F%2Fbeap%2Dbc%2Eyahoo%2Ecom%2Fyc%2FYnY9MS4wLjAmYnM9KDE3YzQxMTRzZShnaWQkVjRSNGtnQUFBQURRZzdGMk0yWEJ5STlGeTRMdEMxWUxqYzBBQkFJMixzdCQxNDQzNTk3NzczMjYyNTgzLHNpJDI4MTU2MSxzcCQxMTk3NzIxNTU2LGN0JDI1LHlieCRTSENFcHB6VGNTZUhtdElYTk9oeEVnLGxuZyRpZCxjciQxMjc5NjE2NTYxLHYkMi4wLGFpZCRJSU5CeDJvS256Yy0sYmkkMjI1NDg1NTYxLG1tZSQ0MDM0MTYyMTExODAwNjY3MTg4LHIkMCx5b28kMSxhZ3AkMjY5NDAxNDQ5LGFwJExSRUMpKQ%2F2%2F%2A%24&u=https%3A%2F%2Fs.yimg.com%2Frq%2Fdarla%2F2-8-9%2Fhtml%2Fr-sf.html&P=guid%3aUwwU2Dk4LjGlqRWpVWAPKQ0mMjAzLgAAAADMVwIP%3bspid%3a978500489%3bypos%3aLREC%7cV4R4kgAAAADQg7F2M2XByI9Fy4LtC1YLjc0ABAI2%7c1197721556%7cLREC%7c1443597773.863743%7c%24%7bSECURE-DARLA%7d&S=ros&i=302388&uccc=%24%7bUCCC%7d&ypos=SKY&D=smpv%3d3%26ed%3dzAomdEO8l1sCgrwd_9juTgNNtfpqobntUlFhpQNBMyNiZ_.6Ng--&_salt=4174748273&B=10&H=&M=3&r=0", info);
    }
}

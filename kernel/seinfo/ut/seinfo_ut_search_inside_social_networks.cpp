#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc51() {
    // search inside social networks
    {
        TInfo info(SE_RIA, ST_MUSIC, "ford focus", ESearchFlags(SF_LOCAL | SF_SOCIAL | SF_SEARCH));

        // vkontakte
        info.Name = SE_VKONTAKTE;
        KS_TEST_URL("http://vk.com/audio?q=ford%20focus", info);
        info.Type = ST_VIDEO;
        KS_TEST_URL("http://vk.com/video?q=ford%20focus&section=search", info);
        info.Type = ST_SOCIAL;
        KS_TEST_URL("http://vk.com/feed?q=ford%20focus&section=search", info);

        // facebook
        info.Name = SE_FACEBOOK;
        info.Type = ST_SOCIAL;
        KS_TEST_URL("http://www.facebook.com/search/results.php?q=ford%20focus&init=quick&tas=0.8147837878204882", info);
        info.Type = ST_PEOPLE;
        KS_TEST_URL("http://www.facebook.com/search/results.php?q=ford%20focus&type=users&init=quick&tas=0.8147837878204882", info);
        info.Type = ST_SOCIAL;
        info.Query = "ivan petrov";
        info.Flags = ESearchFlags(SF_LOCAL | SF_SOCIAL | SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("https://m.facebook.com/findfriends/search/?refid=7&ref=wizard&q=ivan+petrov&submit=%D0%9F%D0%BE%D0%B8%D1%81%D0%BA", info);
        info.Query = "funny cats";
        KS_TEST_URL("https://m.facebook.com/#!/search/?query=funny%20cats&__user=100006360913520", info);

        // twitter
        info.Name = SE_TWITTER;
        info.Type = ST_SOCIAL;
        info.Flags = ESearchFlags(SF_LOCAL | SF_SOCIAL | SF_SEARCH);
        info.Query = "ford focus";
        KS_TEST_URL("http://twitter.com/search?q=ford%20focus&src=typd", info);
        KS_TEST_URL("https://twitter.com/search?q=ford%20focus&src=typd", info);
    }
}

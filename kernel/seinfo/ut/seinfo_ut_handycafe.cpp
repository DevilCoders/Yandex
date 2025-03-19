#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc21() {
    // handycafe
    {
        TInfo info(SE_HANDYCAFE, ST_WEB, "fb", SF_SEARCH);
        KS_TEST_URL("http://search.handycafe.com/results?q=fb&utm_source=HandyCafe&utm_medium=logo_dz&utm_campaign=logo&l=dz&hl=ar&s=logo", info);
        KS_TEST_URL("http://search.handycafe.com/results?q=fb&utm_source=HandyCafe&utm_medium=logo_dz&utm_campaign=logo&l=dz&hl=ar&s=logo&c=web", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://search.handycafe.com/results?q=fb&utm_source=HandyCafe&utm_medium=logo_dz&utm_campaign=logo&l=dz&hl=ar&s=logo&c=images", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://search.handycafe.com/results?q=fb&utm_source=HandyCafe&utm_medium=logo_dz&utm_campaign=logo&l=dz&hl=ar&s=logo&c=video", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://search.handycafe.com/results?q=fb&utm_source=HandyCafe&utm_medium=logo_dz&utm_campaign=logo&l=dz&hl=ar&s=logo&c=news", info);

        info.Type = ST_WEB;
        info.Query = "Nudeactresspictures disha parma";
        info.Flags = ESearchFlags(SF_SEARCH | SF_MOBILE);
        KS_TEST_URL("http://search.handycafe.com/m/search.php?utm_source=HandyCafe&utm_medium=mobile_en&utm_campaign=mobile_top&hl=en&l=se&s=mobile&c=web&q=Nudeactresspictures+disha+parma", info);
    }
}

#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc22() {
    // searchya
    {
        TInfo info(SE_SEARCHYA, ST_WEB, "боевик", SF_SEARCH);
        KS_TEST_URL("http://www.searchya.com/?f=2&uref=1&a=sd-dfl&cd=&cr=&q=%D0%B1%D0%BE%D0%B5%D0%B2%D0%B8%D0%BA", info);
        KS_TEST_URL("http://www.searchya.com/results.php?q=%D0%B1%D0%BE%D0%B5%D0%B2%D0%B8%D0%BA&category=web&a=sd-dfl&f=2&uref=1&sr=&start=1", info);
        KS_TEST_URL("http://www.searchya.com/dfu?q=%D0%B1%D0%BE%D0%B5%D0%B2%D0%B8%D0%BA&category=web&a=sd-dfl&f=2&uref=1&sr=&start=1", info);

        info.Type = ST_IMAGES;
        KS_TEST_URL("http://www.searchya.com/results.php?q=%D0%B1%D0%BE%D0%B5%D0%B2%D0%B8%D0%BA&category=images&a=sd-dfl&f=2&uref=1&sr=&start=1", info);

        info.Type = ST_VIDEO;
        KS_TEST_URL("http://www.searchya.com/results.php?q=%D0%B1%D0%BE%D0%B5%D0%B2%D0%B8%D0%BA&category=video&a=sd-dfl&f=2&uref=1&sr=&start=1", info);

        info.Type = ST_NEWS;
        KS_TEST_URL("http://www.searchya.com/results.php?q=%D0%B1%D0%BE%D0%B5%D0%B2%D0%B8%D0%BA&category=news&a=sd-dfl&f=2&uref=1&sr=&start=1", info);
    }
}

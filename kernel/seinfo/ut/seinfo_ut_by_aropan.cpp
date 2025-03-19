#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc78() {
    // by aropan@
    {
        const TInfo info = GetSeInfo("https://www.google.com/search?q=site:periparking.pl&sourceid=opera&num=0&ie=utf-8&oe=utf-8&qscrl=1&gws_rd=ssl#q=site:periparking.pl&qscrl=1&start=10");
        const TMaybe<ui32> pNum = info.GetPageNum();
        Y_UNUSED(pNum);
    }
}

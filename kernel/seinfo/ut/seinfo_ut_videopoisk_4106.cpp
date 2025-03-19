#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc77() {
    // VIDEOPOISK-4106
    {
        TInfo info(SE_VKONTAKTE, ST_VIDEO, "как доить корову", ESearchFlags(SF_SEARCH | SF_SOCIAL | SF_LOCAL));

        KS_TEST_URL("http://vk.com/video?q=как%20доить%20корову&section=search", info);

        KS_TEST_URL("http://vk.com/video?section=search&z=video-68805175_168061802", TInfo());

        info.Flags = ESearchFlags(SF_SEARCH | SF_SOCIAL | SF_LOCAL | SF_FAKE_SEARCH);
        KS_TEST_URL("http://vk.com/video?q=как%20доить%20корову&section=search&z=video-68805175_168061802", info);
        UNIT_ASSERT(!IsSe("http://vk.com/video?q=как%20доить%20корову&section=search&z=video-68805175_168061802"));
    }
}

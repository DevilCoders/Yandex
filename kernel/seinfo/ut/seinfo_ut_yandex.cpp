#include "seinfo_ut.h"

using namespace NSe;

void TestSeFunc1() {
    {
        TInfo unknown;

        // yandex

        KS_TEST_URL("yandex.ru", unknown);
        KS_TEST_URL("http://yandex.ru", unknown);
        KS_TEST_URL("https://yandex.ru", unknown);
        KS_TEST_URL("https://www.google.another.anyhost.com/search?q=test", unknown);
    }
}

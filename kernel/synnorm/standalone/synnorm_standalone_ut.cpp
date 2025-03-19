#include "synnorm_standalone.h"

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(SynnormStandalone) {
    Y_UNIT_TEST(Get) {
        auto& ref = NSynNorm::GetStandAloneSynnormNormalizer();
        Y_UNUSED(ref);
    }

    Y_UNIT_TEST(Call) {
        auto& ref = NSynNorm::GetStandAloneSynnormNormalizer();
        TString res = ref.NormalizeUTF8("my country and city and town are good great best or bad poor worst");
        UNIT_ASSERT_NO_DIFF(res, "bad best city country good great poor town worst");

        TString res2 = ref.NormalizeUTF8("картошка картофель дом строение здание постройка");
        UNIT_ASSERT_NO_DIFF(res2, "дом клуб клуб тро тро тро");
    }
}

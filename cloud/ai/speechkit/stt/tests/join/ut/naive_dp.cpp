//
// Created by Mikhail Yutman on 04.05.2020.
//

#include <cloud/ai/speechkit/stt/lib/join/naive_dp.h>

#include <library/cpp/testing/unittest/registar.h>

Y_UNIT_TEST_SUITE(TCalcDpLayer) {
    TText zeroBit(
        {"я", "помню", "чудное", "мгновенье", "передо", "мной"},
        0);

    TText firstBit(
        {"чудное", "мгновенье", "передо", "мной", "явилась", "ты"},
        0);

    TLevenshteinDistance levenshteinDistance;

    Y_UNIT_TEST(TestCalculateStartDpLayer) {
        TDpValues<ui32> dpLayer(10, 10);
        TDpBestStates prLayer(10, 10);

        CalculateStartDpLayer<ui32>(
            levenshteinDistance,
            zeroBit,
            firstBit,
            dpLayer,
            prLayer);

        for (int i = 0; i < 10; i++) {
            for (int j = 0; j < 10; j++) {
                Cerr << dpLayer.Values[i][j] << " ";
            }
            Cerr << Endl;
        }
    }
}

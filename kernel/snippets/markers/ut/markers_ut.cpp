#include <library/cpp/testing/unittest/registar.h>

#include <kernel/snippets/markers/markers.h>

namespace NSnippets {

Y_UNIT_TEST_SUITE(TMarkersTest) {

Y_UNIT_TEST(DumpParseReplace) {
    TMarkersMask src;
    for (size_t m = 1; m < 512; m += 2)
        src.Set(m);

    TString markers = "SnipDebug=report=www;snip_width=550;";
    markers += src.Dump();
    markers += "reps=all_on;uil=ru;wantctx=da;dqsigml=4;";

    UNIT_ASSERT_EQUAL(markers, "SnipDebug=report=www;snip_width=550;SnipMarkers=eJzjYGBiAINVFAIAaKoqiw==reps=all_on;uil=ru;wantctx=da;dqsigml=4;");

    TMarkersMask dst;
    dst.Parse(markers);

    UNIT_ASSERT_EQUAL(src, dst);

    src.Set(0);
    UNIT_ASSERT_UNEQUAL(src, dst);

    ReplaceMarkers(markers, src);
    dst.Parse(markers);

    UNIT_ASSERT_EQUAL(src, dst);
}

Y_UNIT_TEST(SparsedDataSize) {
    TMarkersMask src;
    src.Set(512);
    UNIT_ASSERT_EQUAL(src.Dump(), "SnipMarkers=eJzjcGBioApghNIAF38ATA==");
    for (size_t i = 0; i < 512; ++i)
        src.Set(i);
    UNIT_ASSERT_EQUAL(src.Dump(), "SnipMarkers=eJzjcGBiAIP/FAJGiDEMAC31QAw=");
}

Y_UNIT_TEST(WinProbabilityCalculation) {
    UNIT_ASSERT_DOUBLES_EQUAL(WinProbability(TVector<double>()), 0.5, 1E-7);

    UNIT_ASSERT_DOUBLES_EQUAL(WinProbability(TVector<double>(1, 0.7)), 0.7, 1E-7);

    TVector<double> twoProbs;
    twoProbs.push_back(0.3);
    twoProbs.push_back(0.7);
    UNIT_ASSERT_DOUBLES_EQUAL(WinProbability(twoProbs), 0.5, 1E-7);

    UNIT_ASSERT_DOUBLES_EQUAL(WinProbability(TVector<double>(100, 0.5)), 0.5, 1E-7);

    UNIT_ASSERT_DOUBLES_EQUAL(WinProbability(TVector<double>(1000, 0.4)), 0., 1E-7);

    UNIT_ASSERT_DOUBLES_EQUAL(WinProbability(TVector<double>(1000, 0.6)), 1.0, 1E-7);
}

}

}

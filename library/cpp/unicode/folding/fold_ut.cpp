#include "fold.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/string/util.h>
#include <util/string/vector.h>

Y_UNIT_TEST_SUITE(TUnicodeFoldingTest){
    void DoTestUF(ELanguage lang, bool lower, TString in, TString exp, TString expoffs){
        using namespace NUF;
TUtf16String win = UTF8ToWide<true>(in.data(), in.size());

TNormalizer n(lang);
n.SetInput(win);
n.SetDoLowerCase(lower);
n.SetDoRenyxa(true);
n.SetDoSimpleCyr(true);
n.SetFillOffsets(true);
n.DoNormalize();

const TOffsets& offs = n.GetOffsetsInCanonDenormalizedInput();
TString resoffs = JoinStrings(offs.begin(), offs.end(), ",");

in.clear();
WideToUTF8(n.GetOutput(), in);
UNIT_ASSERT_VALUES_EQUAL_C(in, exp, Sprintf("(\"%s\" != \"%s\")", in.data(), exp.data()).data());
UNIT_ASSERT_VALUES_EQUAL_C(resoffs, expoffs,
                           (WideToUTF8(ToWtring(n.GetCanonDenormalizedInput()).Quote()) + " <> " + WideToUTF8(ToWtring(n.GetOutput()).Quote()) + " in " + ToString(n.GetCanonDenormalizedInput().size()) + " (" + in + ")").data());
}

Y_UNIT_TEST(TestUF) {
    DoTestUF(LANG_RUS, true, "", "", "");
    DoTestUF(LANG_RUS, true, "\n\t\t\n\n\n", " ", "0");
    DoTestUF(LANG_RUS, true, TString("\0\0\0", 3), " ", "0");
    DoTestUF(LANG_RUS, false, "Йо\u00ADж —   ёж. \u2000\u2009 йО ", "Йож - еж. йО ", "0,2,4,5,6,7,10,12,13,14,18,20,21");
    DoTestUF(LANG_RUS, true, "Йо\u00ADж —   ёж. \u2000\u2009 йО ", "йож - еж. йо ", "0,2,4,5,6,7,10,12,13,14,18,20,21");
    DoTestUF(LANG_BEL, true, "08 жніўня", "08 жнiвня", "0,1,2,3,4,5,6,8,9");
    DoTestUF(LANG_BEL, true, "08 сьнежань", "08 cнежань", "0,1,2,3,5,6,7,8,9,10");
    DoTestUF(LANG_CZE, true, "08 února", "08 unora", "0,1,2,3,5,6,7,8");
    DoTestUF(LANG_UKR, true, "08 Ciч", "08 ciч", "0,1,2,3,4,5");
    DoTestUF(LANG_BEL, true, "08 верасьня", "08 вераcня", "0,1,2,3,4,5,6,7,9,10");
    DoTestUF(LANG_UKR, true, "08 травень", "08 травень", "0,1,2,3,4,5,6,7,8,9");
    DoTestUF(LANG_TUR, true, "08 Eylül", "08 eylul", "0,1,2,3,4,5,6,8");
}
}
;

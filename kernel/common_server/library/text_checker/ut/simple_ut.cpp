#include <kernel/common_server/library/text_checker/checker.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>

using namespace NCS;
using namespace NCS::NTextChecker;
Y_UNIT_TEST_SUITE(TextChecker) {

    const TString baseDeniedLines = R"(
слава украине
помощь, всу;
всу рулит;
бандера рулит;
слава слава;
на поддержку ВСУ;
Ура!
копнов павел
котором приемами
support of the armed forces of Ukraine
support ofa the armed forces ofa Ukraine
)";

    const TString Delimiters = " ,.;:\"'";

    Y_UNIT_TEST(SimpleUnsplitted) {
        TTextChecker checker(3, false, Delimiters, false);
        checker.Compile(StringSplitter(baseDeniedLines).SplitBySet("\n").ToList<TString>());
        TRequestFeatures rFeaturesWords;
        rFeaturesWords.SetUnorderedWordsMax(1);
        TRequestFeatures rFeaturesMisspell;
        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("слава укроине не вызывает вопросов", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("support ofф the armed forces of Ukraine", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(2);
        CHECK_WITH_LOG(checker.CheckSubstrings("support ofф the arвmed forces of Ukraine", rFeaturesMisspell));
        //        CHECK_WITH_LOG(checker.CheckSubstrings("support фofф the armed forces of Ukraine", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("support of the adrmeddd forces of Ukraine", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("ddsupport of ddthe armed forces do Ukraine", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(!checker.CheckSubstrings("слава укроне не вызывает вопросов", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("support of the armed forces of Ukraine", rFeaturesMisspell));
        

    }

    Y_UNIT_TEST(SimpleNormalize) {
        TTextChecker checker(3, true, Delimiters, true);
        checker.Compile(StringSplitter(baseDeniedLines).SplitBySet("\n").ToList<TString>());
        TRequestFeatures rFeaturesWords;
        rFeaturesWords.SetUnorderedWordsMax(1);
        TRequestFeatures rFeaturesMisspell;

        rFeaturesMisspell.SetMisspellsCountMax(1).SetUnorderedWordsMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("asdf df support of armed the forces of Ukraine sdfsdf sdf sdfsdfewdf23d 3", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(1).SetUnorderedWordsMax(0);
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод во славу Украины", rFeaturesMisspell));
    }

    Y_UNIT_TEST(Simple) {
        TTextChecker checker(3, true, Delimiters, false);
        checker.Compile(StringSplitter(baseDeniedLines).SplitBySet("\n").ToList<TString>());
        TRequestFeatures rFeaturesWords;
        rFeaturesWords.SetUnorderedWordsMax(1);
        TRequestFeatures rFeaturesMisspell;

        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(!checker.CheckSubstrings("перевод во славу Украины", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("бандере сделать рулят", rFeaturesMisspell));

        rFeaturesMisspell.SetMisspellsCountMax(2).SetNearWordsDistanceMax(1);
        CHECK_WITH_LOG(!checker.CheckSubstrings("support фofф the аrmed forces of Ukraine", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("support фofaф the аrmed forces ofa Ukraine", rFeaturesMisspell));
        rFeaturesMisspell.SetNearWordsDistanceMax(0);
        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("support ofф the аrmed forces of Ukraine", rFeaturesMisspell));

        CHECK_WITH_LOG(checker.CheckSubstrings("support of the armed forces of Ukraine"));
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. всу помощь", rFeaturesWords));
        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(!checker.CheckSubstrings("котовромв приемамви", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("слава ываывсвукраине", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("слава украинеы", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("слава укруине", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("слава украинеываываы", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("всу релит", rFeaturesMisspell));

        rFeaturesMisspell.SetNearWordsDistanceMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("бандере сделать рулят", rFeaturesMisspell));
        rFeaturesMisspell.SetNearWordsDistanceMax(0);

        rFeaturesMisspell.SetMisspellsCountMax(0);
        CHECK_WITH_LOG(!checker.CheckSubstrings("копновым павлом", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("лав лав", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(3);
        CHECK_WITH_LOG(checker.CheckSubstrings("копновым павлом", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(0);
        CHECK_WITH_LOG(!checker.CheckSubstrings("копновым павлом", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("ура", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("слава слава", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("слава укруине", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("слава украине не вызывает вопросов", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(5);
        CHECK_WITH_LOG(!checker.CheckSubstrings("слава ываывсвукраине", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("слава украинеываываы", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. всу помощь"));
        CHECK_WITH_LOG(!checker.CheckSubstrings("перевод. помощь всю"));
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. помощь всю", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("перевод. помощь взю", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("всунуть релит", rFeaturesMisspell));
        rFeaturesMisspell.SetMisspellsCountMax(4);
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. помощь взю", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. помощь всу"));
        CHECK_WITH_LOG(!checker.CheckSubstrings("перевод. всунуть помощь"));

        TRequestFeatures rFeaturesDistance;
        rFeaturesDistance.SetNearWordsDistanceMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. слава в украине"));
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод. слава в украине", rFeaturesDistance));

        rFeaturesMisspell.SetMisspellsCountMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("перевод во славу украины", rFeaturesMisspell));
        CHECK_WITH_LOG(!checker.CheckSubstrings("бандерлоги рулят", rFeaturesMisspell));
        rFeaturesMisspell.SetUnorderedWordsMax(1);
        CHECK_WITH_LOG(checker.CheckSubstrings("рулит бандеры", rFeaturesMisspell));
        CHECK_WITH_LOG(checker.CheckSubstrings("слава вукраине", rFeaturesMisspell));

    };
}

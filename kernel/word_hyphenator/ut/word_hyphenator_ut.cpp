#include <tuple>
#include <util/generic/string.h>
#include <util/charset/wide.h>
#include <kernel/word_hyphenator/word_hyphenator.h>
#include <library/cpp/testing/unittest/registar.h>

std::tuple<TString, TString> TestPairs[] = { {"круглый", "круг\u00ADлый"},
                                           {"молоко", "мо\u00ADло\u00ADко"},
                                           {"вскользь", "вскользь"},
                                           {"зайка", "зай\u00ADка"},
                                           {"подъезд", "подъ\u00ADезд"},
                                           {"адъюнктство", "адъ\u00ADюнктст\u00ADво"},
                                           {"лауэит", "лауэ\u00ADит"},
                                           {"центральный", "цен\u00ADтраль\u00ADный"},
                                           {"идея", "идея"},
                                           {"мама папа", "ма\u00ADма па\u00ADпа"},
                                           {"эхо скот сова", "эхо скот со\u00ADва"},
                                           {"", ""},
                                           {"я", "я"},
                                           {"нововведение", "но\u00ADво\u00ADвве\u00ADде\u00ADние"},
                                           {"спецклиника", "спец\u00ADкли\u00ADни\u00ADка"},
                                           {"строительство", "стро\u00ADительст\u00ADво"},
                                           {"акация", "ака\u00ADция"},
                                           {"июля", "июля"},
                                           {"июль", "июль"},
                                           {"вероисповедание", "ве\u00ADро\u00ADис\u00ADпо\u00ADве\u00ADда\u00ADние"},
                                           {"молодой", "мо\u00ADло\u00ADдой"},
                                           {"винегрет", "ви\u00ADнег\u00ADрет"},
                                           {"предпросмотр", "пред\u00ADпро\u00ADсмотр"},
                                           {"статья", "ста\u00ADтья"},
                                           {"предотвратить", "пред\u00ADотв\u00ADра\u00ADтить"},
                                           {"постройка", "по\u00ADстрой\u00ADка"},
                                           {"пододеяльник", "под\u00ADоде\u00ADяль\u00ADник"},
                                           {"опытными", "опыт\u00ADны\u00ADми"},
                                           {"ЮНЕСКО", "ЮНЕСКО"},
                                           {"ЯрГУ", "ЯрГУ"}};

Y_UNIT_TEST_SUITE(TWordHyphenatorTest) {
    Y_UNIT_TEST(StrokaTest) {
       TWordHyphenator hyphenator;

       for(const auto& pair : TestPairs) {
            auto strokaResult = hyphenator.Hyphenate(std::get<0>(pair));
            UNIT_ASSERT_VALUES_EQUAL(std::get<1>(pair),strokaResult);
        }
    }

    Y_UNIT_TEST(WtrokaTest) {
       TWordHyphenator hyphenator;

       for(const auto& pair : TestPairs) {
            auto wtrokaResult = hyphenator.Hyphenate(UTF8ToWide(std::get<0>(pair)));
            UNIT_ASSERT_VALUES_EQUAL(UTF8ToWide(std::get<1>(pair)),wtrokaResult);
        }
    }

}

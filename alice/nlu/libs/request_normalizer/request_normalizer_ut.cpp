#include "request_normalizer.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

#include <library/cpp/resource/resource.h>

using namespace NNlu;

namespace {
    void TestCommon(ELanguage lang, const TString& text, const TString& expected) {
        const TString actual = TRequestNormalizer::Normalize(lang, text);
        if (expected == actual) {
            return;
        }
        TStringBuilder message;
        message << Endl;
        message << "TRequestNormalizer error:" << Endl;
        message << "  text:     " << text << Endl;
        message << "  expected: " << expected << Endl;
        message << "  actual:   " << actual << Endl;
        UNIT_FAIL(message);
    }
}

Y_UNIT_TEST_SUITE(TRequestRuNormalizer) {

    void Test(const TString& text, const TString& expected) {
        TestCommon(LANG_RUS, text, expected);
    }

    Y_UNIT_TEST(NumberName) {
        Test("2",                                   "2");
        Test("два",                                 "2");
        Test("второй",                              "2");
        Test("2-й",                                 "2 - й");
        Test("двойка",                              "двойка"); // bad
        Test("тройка",                              "тройка"); // bad
        Test("пара",                                "пара"); // bad
        Test("двое",                                "двое"); // bad
        Test("двойной",                             "двойной");
        Test("двоичный",                            "двоичный");
        Test("ноль",                                "0");
        Test("нуль",                                "0");
        Test("сто",                                 "100");
        Test("сотня",                               "сотня");
        Test("три сотни",                           "3 сотни");
    }

    Y_UNIT_TEST(ComposeNumbers) {
        Test("100 11 2",                            "100 11 2");
        Test("сто 11 2",                            "100 11 2");
        Test("СТО 11 2",                            "100 11 2");
        Test("СТО-11 2",                            "100 - 11 2");
        Test("100 11-й 2",                          "100 11 - й 2");
        Test("сто 11-й 2",                          "100 11 - й 2");
        Test("сотый 11 2",                          "100 11 2");
        Test("сотый одиннадцать два",               "100 11 2");
        Test("сто одиннадцать два",                 "111 2");
        Test("сто одиннадцатый второй",             "111 2");
        Test("сотый десятый первый",                "100 10 1");
        Test("десять тысяча сто десять один",       "10 1110 1");
        Test("1 1000000",                           "1 1000000");
        Test("101 1000000",                         "101 1000000");
        Test("5 1000000",                           "5 1000000");
        Test("105 1000000",                         "105 1000000");
        Test("1 1000",                              "1 1000");
        Test("101 1000",                            "101 1000");
    }

    Y_UNIT_TEST(NumberCase) {
        Test("Nom. Есть семь тысяч четыреста семьдесят один рубль",                  "nom есть 7471 рубль");
        Test("Gen. Нет семи тысяч четырехсот семидесяти одного рубля",               "gen нет 7471 рубля");
        Test("Dat. Дать семи тысячам четыремстам семидесяти одному жителю",          "dat дать 7471 жителю");
        Test("Acc. Винить семь тысяч четыреста семьдесят одного жителя",             "acc винить 7471 жителя");
        Test("Ins. Доволен семью тысячами четырьмястами семьюдесятью одним жителем", "ins доволен 7471 жителем"); // Ins?
        Test("Loc. Думать о семи тысячах четырехстах семидесяти одном жителе",       "loc думать о 7471 жителе");
    }

    Y_UNIT_TEST(NumberWrongCase) {
        Test("К трёмстам семидесяти одной тысяче",  "к 371000");
        Test("К трёмстам семьдесят одной тысяче",   "к 371000");
        Test("К шестистам семидесяти одному",       "к 671");
        Test("К шестьсот семидесяти одному",        "к 671");
        Test("К шестьсот семьдесят одному",         "к 671");
        Test("К шестистам семьдесят одному",        "к 671");
        Test("К шестистам семьдесят один",          "к 600 71");
    }

    Y_UNIT_TEST(Time) {
        Test("без двадцати восьми семь",            "без 28 7");
        Test("без двадцати восемь семь",            "без 20 8 7");
        Test("двадцать ноль ноль",                  "20 0 0");
        Test("двадцать ноль пять",                  "20 0 5");
        Test("20:00",                               "20:00");
        Test("20:05",                               "20:05");
        Test("полседьмого",                         "полседьмого"); // bad
        Test("6:30",                                "6:30");
    }

    Y_UNIT_TEST(Phone) {
        Test("+7 495 500 50 03",                    "+7(495)500-50-03");
        Test("+7(495)500-50-03",                    "+ 7 495 500 - 50 - 03"); // bad
    }

    Y_UNIT_TEST(Decompouse) {
        Test("АИ95",                                "аи 95");
        Test("Брат-2",                              "брат - 2"); // bad
    }

    Y_UNIT_TEST(Ordinal) {
        Test("второй",                              "2");
        Test("2-й",                                 "2 - й");
        Test("2'й",                                 "2 й");
        Test("2й",                                  "2 й");
        Test("2ой",                                 "2 ой");
        Test("2-ой",                                "2 - ой");
        Test("нулевой",                             "нулевой"); // bad
        Test("нолевой",                             "нолевой");
        Test("шесть тысяч сто двадцать третий",     "6123");
        Test("шеститысячный",                       "6000");
        Test("двадцатипятитысячный",                "20 5000"); // error
        Test("стосорокашестисотмиллионный",         "стосорокашестисотмиллионный"); // error
        Test("тридцатиодномиллиардный",             "31000000000");
        Test("первый раз в первый класс",           "1 раз в 1 класс");
        Test("мне на тысячу девяносто второго",     "мне на 1090 2");
    }

    Y_UNIT_TEST(Tokens) {
        Test("Первый, второй; третий. Четвёртый?",  "1 2 3 4");
        Test("Три-четыре.",                         "три-четыре");
        Test("1, 2, 3, 4.",                         "1 2 3 4");
        Test("1,2,3,4.",                            "1,2 3,4");
        Test("1\t2\n3 4",                           "1 2 3 4");
    }

    Y_UNIT_TEST(Fraction) {
        Test("две вторых",                          "2/2");
        Test("два вторых",                          "2 2");
        Test("треть",                               "треть");
        Test("половина",                            "половина");
        Test("три с половиной",                     "3,5");
        Test("три целых шесть восьмых",             "3 целых 6/8");
        Test("три целых и шесть восьмых",           "3 целых и 6/8");
        Test("восемь целых шесть десятых",          "8,6");
        Test("три и пять восьмых",                  "3 и 5/8");
        Test("восемь целых шестьдесят сотых",       "8,60");
        Test("восемь целых одиннадцать сотых",      "8,11");
        Test("восемь целых одиннадцать тысячных",   "8,011");
        Test("восемь целых одиннадцать десятых",    "8 целых 11/10");
        Test("полтора",                             "полтора"); // bad
        Test("полтора часа",                        "полтора часа");
        Test("полтораста",                          "полтораста");
        Test("полторы тысячи",                      "1500");
        Test("три с половиной тысячи",              "3500");
        Test("две пятьсот",                         "2 500");
        Test("двигатель один и шесть",              "двигатель 1 и 6");
    }

    Y_UNIT_TEST(Float) {
        Test("8.5, 3.1. 7,1.",                      "8.5 3.1 7,1");
        Test("-8.5, -3.1. -7,1.",                   "-8.5 -3.1 -7,1");
        Test("-8.5,-3.1.-7,1.",                     "-8.5 -3.1 -7,1");
    }

    Y_UNIT_TEST(LongNumber) {
        Test("Четыреста семьдесят восемь миллиардов пятьсот одиннадцать тысяч семь",
             "478000511007");
        Test("Грюнвальдская битва произошла в тысяча четыреста десятом году",
             "грюнвальдская битва произошла в 1410 году");
        Test("Николай II правил с тысяча восемьсот девяносто четвёртого года по тысяча девятьсот семнадцатый год",
             "николай ii правил с 1894 года по 1917 год");
    }

    Y_UNIT_TEST(Calc) {
        Test("Сколько будет дважды два",            "сколько будет 2 * 2");
        Test("минус три плюс пять",                 "минус 3 плюс 5");
        Test("2 + 2 = 4",                           "2 + 2 = 4");
        Test("2 - 2 = 0",                           "2 - 2 = 0");
        Test("2+2 = 4",                             "2 + 2 = 4");
        Test("-3 - 2 = -5",                         "-3 - 2 = -5");
        Test("трижды три",                          "3 * 3");
        Test("четырежды четыре",                    "4 * 4");
        Test("пятью тысяча пятьсот",                "5 * 1500");
        Test("шестью восемь",                       "6 * 8");
        Test("семью четыреста шестьдесят восемь",   "7 * 468");
        Test("восемью сорок восемь",                "8 * 48");
        Test("девятью девять",                      "9 * 9");
        Test("десятью десять",                      "10 * 10");
        Test("пятью столетиями ранее",              "5 столетиями ранее");
        Test("расплатился пятью тысячами",          "расплатился 5000");
        Test("пятью сто",                           "5 * 100");
        Test("семью семью восемь",                  "7 * 7 * 8");
        Test("шестью",                              "6");
    }

    Y_UNIT_TEST(Wether) {
        Test("На улице -5",                         "на улице -5");
        Test("На улице +5",                         "на улице + 5");
        Test("На улице - 5",                        "на улице - 5");
        Test("На улице + 5",                        "на улице + 5");
        Test("На улице минус пять градусов",        "на улице минус 5 градусов");
        Test("Плюс семь градусов тепла",            "плюс 7 градусов тепла");
        Test("Завтра в Москве минус восемь",        "завтра в москве минус 8");
        Test("Сегодня плюс двадцать один",          "сегодня плюс 21");
    }

    Y_UNIT_TEST(Diacritics) {
        Test("четвёртый",                           "4");
        Test("четвертый",                           "4");
        Test("всё",                                 "всё");
    }

    Y_UNIT_TEST(Accent) {
        Test("со́рок - имя числительное",            "со рок - имя числительное"); // error
        Test("сорок - имя числительное",            "40 - имя числительное");
    }

    Y_UNIT_TEST(Address) {
        Test("дом восемь дробь два",                "дом 8 / 2");
        Test("дом 8/2",                             "дом 8 / 2");
        Test("число 1/2",                           "число 1 / 2");
        Test("дом восемь дробь а",                  "дом 8 дробь а");
        Test("дом 8/а",                             "дом 8 / а");
        Test("5 м/с",                               "5 м/с");
    }

    Y_UNIT_TEST(Units) {
        Test("5 м/с",                               "5 м/с");
        Test("5 р",                                 "5 р");
        Test("$5",                                  "$ 5");
        Test("шесть долларов",                      "6 долларов");
        Test("5 dollars",                           "5 dollars");
        Test("Пять процентов",                      "5 процентов");
        Test("5 процентов",                         "5 процентов");
        Test("5 %",                                 "5 %");
        Test("5%",                                  "5 %");
    }

    Y_UNIT_TEST(Bad) {
        Test("Бля, блядь",                          "бля блядь");
        Test("OpenFST fix: ε ε-величина εxxx",      "openfst fix e e-величина exxx"); // bad
        Test("α-частица",                           "- -частица"); // error
    }
}

Y_UNIT_TEST_SUITE(TRequestTrNormalizer) {

    void Test(const TString& text, const TString& expected) {
        TestCommon(LANG_TUR, text, expected);
    }

    Y_UNIT_TEST(NumberName) {
        Test("2",                                   "2");
        Test("iki",                                 "2");
        Test("ikinci",                              "2."); // error
        Test("eksi iki",                            "eksi 2");
        Test("sifir",                               "sifir"); // error
        Test("yüz",                                 "100");
        Test("üç yüz",                              "300");
        Test("bin",                                 "1000");
    }

    Y_UNIT_TEST(ComposeNumbers) {
        Test("yüz bin beş yüz yedi",                "100507");
        Test("üç milyon sekiz yüz yirmi iki bin",   "3822000");
    }
}

Y_UNIT_TEST_SUITE(TRequestArNormalizer) {

    void Test(const TString& text, const TString& expected) {
        TestCommon(LANG_ARA, text, expected);
    }

    Y_UNIT_TEST(ArabicText) {
        Test("", "");
        Test("TeXt 9:00 أليسا، اضبطي المنبه في أيام الأربعاء على الساعة", "text 9:00 اليسا، اضبطي المنبه في ايام الاربعاء على الساعه");
        Test("هَلْ ذَهَبْتَ إِلَى المَكْتَبَةِ ﰷ؟", "هل ذهبت الى المكتبه كا؟");
    }
}

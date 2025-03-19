#include "abc.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/charset/wide.h>

namespace NNormalizerTest {
    static const size_t BufferSize = 56;
    using NLemmer::TAlphabetWordNormalizer;
    using NLemmer::TConvertMode;

    static const char* const RussianText[] = {
        "мiрНЫЙ",
        "ТРуЪ",
        "áдмIнъ",
        "3аЛеЗ",
        "на",
        "Ёлку",
        "c",
        //    "ëжикомъ",  // needs derenyx 0x0BE => 0x451
        nullptr};
    static const char* const RussianLowCase[] = {
        "мiрный, true",
        "труъ, true",
        "áдмiнъ, true",
        "3алез, true",
        "на, true",
        "ёлку, true",
        "c, true",
        "ëжикомъ, true"};

    static const char* const RussianUpCase[] = {
        "МIРНЫЙ, true",
        "ТРУЪ, true",
        "ÁДМIНЪ, true",
        "3АЛЕЗ, true",
        "НА, true",
        "ЁЛКУ, true",
        "C, true",
        "ËЖИКОМЪ, true"};

    static const char* const RussianTitleCase[] = {
        "Мiрный, true",
        "Труъ, true",
        "Áдмiнъ, true",
        "3алез, true",
        "На, true",
        "Ёлку, true",
        "C, true",
        "Ëжикомъ, true"};

    static const char* const RussianPreConvert[] = {
        "мірный, true",
        "труъ, true",
        "адмінъ, true",
        "3алез, true",
        "на, true",
        "ёлку, true",
        "с, true",
        "ёжикомъ, true"};

    static const char* const RussianNormal[] = {
        "мирный, true",
        "труъ, true",
        "адмін, false", // because "i" can't be here
        "3алез, false", // because of numeral "3"
        "на, true",
        "елку, true",
        "с, true",
        "ежиком, true"};

    static const wchar16 wtrInval[] = {'i', 0xFFFF, 's', 't', 0};
    static const TString strInval = WideToUTF8(TWtringBuf(wtrInval)).data();

    static const char* const TurkishText[] = {
        "Istânbul'da",
        "ıstanbul'da",
        "İslâm",
        "islam",
        strInval.data(),
        nullptr};

    static const char* const TurkishLowCase[] = {
        "ıstânbul'da, true",
        "ıstanbul'da, true",
        "islâm, true",
        "islam, true",
        "ist, true"};

    static const char* const TurkishUpCase[] = {
        "ISTÂNBUL'DA, true",
        "ISTANBUL'DA, true",
        "İSLÂM, true",
        "İSLAM, true",
        "İST, true"};

    static const char* const TurkishTitleCase[] = {
        "Istânbul'da, true",
        "Istanbul'da, true",
        "İslâm, true",
        "İslam, true",
        "İst, true"};

    static const char* const TurkishPreConvert[] = {
        "ıstânbul'da, true",
        "ıstanbul'da, true",
        "islâm, true",
        "islam, true",
        "ist, true"};

    static const char* const TurkishNormal[] = {
        "ıstanbulda, true",
        "ıstanbulda, true",
        "islam, true",
        "islam, true",
        "ist, true"};

    class TAlphaRulesTest: public TTestBase {
        UNIT_TEST_SUITE(TAlphaRulesTest);
        UNIT_TEST(TestRussian);
        UNIT_TEST(TestTurkish);
        UNIT_TEST_SUITE_END();

    private:
        void TestRussian() {
            CheckNormalization(LANG_RUS,
                               RussianText,
                               RussianLowCase,
                               RussianUpCase,
                               RussianTitleCase,
                               RussianPreConvert,
                               RussianNormal);
        }
        void TestTurkish() {
            CheckNormalization(LANG_TUR,
                               TurkishText,
                               TurkishLowCase,
                               TurkishUpCase,
                               TurkishTitleCase,
                               TurkishPreConvert,
                               TurkishNormal);
        }
        void CheckNormalization(ELanguage lang, const char* const text[], const char* const low[], const char* const up[], const char* const title[], const char* const pre[], const char* const norm[]) const;
    };

    namespace {
#define TTCaser(Name)                                                                                              \
    struct T##Name {                                                                                               \
        ELanguage Id;                                                                                              \
        const TAlphabetWordNormalizer* Nrm;                                                                        \
        T##Name(const TAlphabetWordNormalizer* nrm, ELanguage id)                                                  \
            : Id(id)                                                                                               \
            , Nrm(nrm) {                                                                                           \
        }                                                                                                          \
        NLemmer::TConvertRet operator()(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const { \
            return Nrm->Name(word, length, converted, bufLen);                                                     \
        }                                                                                                          \
        NLemmer::TConvertRet operator()(TUtf16String& s) const {                                                   \
            return Nrm->Name(s);                                                                                   \
        }                                                                                                          \
    }

        TTCaser(ToLower);
        TTCaser(ToUpper);
        TTCaser(ToTitle);

#undef TTCaser

        struct TNormalize {
            ELanguage Id;
            const TAlphabetWordNormalizer* Nrm;
            const TConvertMode Mode;
            TNormalize(const TAlphabetWordNormalizer* nrm, const TConvertMode& mode, ELanguage id)
                : Id(id)
                , Nrm(nrm)
                , Mode(mode)
            {
            }
            NLemmer::TConvertRet operator()(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const {
                return Nrm->Normalize(word, length, converted, bufLen, Mode);
            }
            NLemmer::TConvertRet operator()(TUtf16String& s) const {
                return Nrm->Normalize(s, Mode);
            }
        };

        struct TNormalize2 {
            ELanguage Id;
            const TAlphabetWordNormalizer* Nrm;
            const TConvertMode Mode1;
            const TConvertMode Mode2;
            TNormalize2(const TAlphabetWordNormalizer* nrm, ELanguage id)
                : Id(id)
                , Nrm(nrm)
                , Mode1(TAlphabetWordNormalizer::ModePreConvert() | TConvertMode(NLemmer::CnvDecompose, NLemmer::CnvLowCase))
                , Mode2(TAlphabetWordNormalizer::ModeConvertNormalized())
            {
            }
            NLemmer::TConvertRet operator()(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const {
                size_t bufLen2 = length * 4 + 1;
                wchar16* buf = (wchar16*)alloca(bufLen2 * sizeof(wchar16));
                NLemmer::TConvertRet ret1 = Nrm->Normalize(word, length, buf, bufLen2, Mode1);
                if (!ret1.Valid) {
                    ret1.Length = Min(ret1.Length, bufLen);
                    memcpy(converted, buf, ret1.Length);
                    return ret1;
                }
                NLemmer::TConvertRet ret2 = Nrm->Normalize(buf, ret1.Length, converted, bufLen, Mode2);
                ret2.IsChanged |= ret1.IsChanged;
                return ret2;
            }

            NLemmer::TConvertRet operator()(TUtf16String& s) const {
                NLemmer::TConvertRet ret1 = Nrm->Normalize(s, Mode1);
                if (!ret1.Valid)
                    return ret1;
                NLemmer::TConvertRet ret2 = Nrm->Normalize(s, Mode2);
                ret2.IsChanged |= ret1.IsChanged;
                return ret2;
            }
        };
    }

    TString PrintRes(const wchar16* buffer, const NLemmer::TConvertRet& ret);
    TString PrintRes(const TUtf16String& buffer, const NLemmer::TConvertRet& ret);

    TString check(const char* to, const wchar16* ws1, const NLemmer::TConvertRet& r1, const TUtf16String& ws2, const NLemmer::TConvertRet& r2) {
        TString outStr;
        TStringOutput out(outStr);

        if (r1.Length != r2.Length)
            out << "different length for buffer and TUtf16String result: " << r1.Length << " " << r2.Length << Endl;
        if (r1.Valid != r2.Valid)
            out << "different validity for buffer and TUtf16String result: " << r1.Valid << " " << r2.Valid << Endl;
        //    if (r1.IsChanged != r2.IsChanged)
        //        out << "different IsChanged for buffer and TUtf16String result" << Endl;

        TString s1 = PrintRes(ws1, r1);
        TString s2 = PrintRes(ws2, r2);

        if (s1 != s2)
            out << "different buffer and TUtf16String result: \"" << s1 << "\" \"" << s2 << "\"" << Endl;

        if (s1 != to)
            out << "\"" << s1 << "\" doesn't match to \"" << to << "\"" << Endl;

        return outStr;
    }

    template <class TConv>
    bool Checker(const TUtf16String& ws, const char* to, const TConv& cnv) {
        wchar16 buffer[BufferSize];
        TUtf16String sbuf = ws;

        NLemmer::TConvertRet ret1 = cnv(ws.data(), ws.size(), buffer, BufferSize);
        NLemmer::TConvertRet ret2 = cnv(sbuf);

        TString serr = check(to, buffer, ret1, sbuf, ret2);
        if (!!serr) {
            Cerr << "While processing string \"" << WideToUTF8(ws) << "\" "
                 << "with \"" << NameByLanguage(cnv.Id) << "\" language:\n\t"
                 << serr;
            return false;
        }
        return true;
    }

    void TAlphaRulesTest::CheckNormalization(ELanguage lang, const char* const text[], const char* const low[], const char* const up[], const char* const title[], const char* const pre[], const char* const norm[]) const {
        const TAlphabetWordNormalizer* nrm = NLemmer::GetAlphaRules(lang);
        UNIT_ASSERT(nrm);

        for (size_t i = 0; text[i]; ++i) {
            TUtf16String s = UTF8ToWide(text[i]);
            UNIT_ASSERT(Checker(s, low[i], TToLower(nrm, lang)));
            UNIT_ASSERT(Checker(s, up[i], TToUpper(nrm, lang)));
            UNIT_ASSERT(Checker(s, title[i], TToTitle(nrm, lang)));
            UNIT_ASSERT(Checker(s, pre[i], TNormalize(nrm, TAlphabetWordNormalizer::ModePreConvert() | TConvertMode(NLemmer::CnvDecompose, NLemmer::CnvLowCase), lang)));
            UNIT_ASSERT(Checker(s, norm[i], TNormalize(nrm, ~TConvertMode(), lang)));
            UNIT_ASSERT(Checker(s, norm[i], TNormalize2(nrm, lang)));
        }
    }

    TString PrintRes_(TString s, bool t) {
        s += ", ";
        s += t ? "true" : "false";
        return s;
    }

    TString PrintRes(const wchar16* buffer, const NLemmer::TConvertRet& ret) {
        TString s = WideToUTF8(buffer, ret.Length);
        return PrintRes_(s, ret.Valid);
    }

    TString PrintRes(const TUtf16String& buffer, const NLemmer::TConvertRet& ret) {
        TString s = WideToUTF8(buffer);
        return PrintRes_(s, ret.Valid);
    }

    UNIT_TEST_SUITE_REGISTRATION(TAlphaRulesTest);
}

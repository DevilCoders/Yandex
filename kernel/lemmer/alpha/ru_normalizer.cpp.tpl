// for using in build_abc_data.py
// vim: set ft=cpp:
#include <library/cpp/token/charfilter.h>
#include <util/charset/wide.h>
#include <util/generic/singleton.h>

namespace NLemmer {
namespace NDetail {
    namespace {
        class TRussianWordNormalizer : public TAlphaStructConstructer<
            TAlphaRussian,
            TEasternSlavicPreConverter,
            TEasternSlavicDerenyxer,
            TGeneralCyrilicConverter,
            TEmptyConverter,
            TEmptyConverter,
            TEmptyConverter,
            TEmptyDiacriticsMap>
        {
        public:
            TRussianWordNormalizer()
                : TAlphaStructConstructer<TAlphaRussian,
                                          TEasternSlavicPreConverter,
                                          TEasternSlavicDerenyxer,
                                          TGeneralCyrilicConverter,
                                          TEmptyConverter,
                                          TEmptyConverter,
                                          TEmptyConverter,
                                          TEmptyDiacriticsMap>()
            { }

        private:
            template<class T>
            void AdvancedConvert_(T& word, NLemmer::TConvertRet& ret) const;
            bool AdvancedConvert(const wchar16* word, size_t length, wchar16* converted, size_t bufLen, NLemmer::TConvertRet& ret) const {
                if (length > bufLen) {
                    ret.Valid = false;
                    length = bufLen;
                }
                memcpy(converted, word, length * sizeof(wchar16));
                ret.Length = length;
                AdvancedConvert_(converted, ret);
                return true;
            }
            bool AdvancedConvert(TUtf16String& s, NLemmer::TConvertRet& ret) const {
                AdvancedConvert_(s, ret);
                if (s.length() > ret.Length)
                    s.resize(ret.Length);
                Y_ASSERT(s.length() == ret.Length);
                return true;
            }
        };
    }

    namespace {
    struct TRussianConvertConsts {
        const wchar16 letYat;
        const wchar16 letDecimalI;
        const wchar16 letHardSign;
        const wchar16 letI;
        const wchar16 letE;
        const wchar16 letM;
        const wchar16 letZ;
        const wchar16 letS;
        const TUtf16String strOne;
        const TUtf16String strOdne;
        const TUtf16String strOdneh;
        const TUtf16String strVozs;
        const TUtf16String strNizs;
        const TUtf16String strRazs;
        const TUtf16String strRozs;
        const TUtf16String strBe;
        const TUtf16String strIz;
        const TUtf16String strChrez;
        const TUtf16String strCherezs;

        const TUtf16String setVocals;
        const TUtf16String setConsonants;
        const TUtf16String setBreathConsonants;
        const TUtf16String setPp;
        TRussianConvertConsts()
            : letYat(0x0463)   // e
            , letDecimalI(0x0456)   // i
            , letHardSign(0x044A)
            , letI(0x0438)
            , letE(0x0435)
            , letM(0x043C)
            , letZ(0x0437)
            , letS(0x0441)
            , strOne(u"онѣ")
            , strOdne(u"однѣ")
            , strOdneh(u"однѣхъ")
            , strVozs(u"возс")
            , strNizs(u"низс")
            , strRazs(u"разс")
            , strRozs(u"розс")
            , strBe(u"бе")
            , strIz(u"из")
            , strChrez(u"чрез")
            , strCherezs(u"черезс")

            , setVocals(u"аaеeиоoуyыэюяйё") // and semivocals
            , setConsonants(u"бвгджзклмнпрстфхцчшщ")
            , setBreathConsonants(u"пткцчфсшщх")
            , setPp(u"рp")
        {};
    };
    }

    static bool isPrefix(const wchar16* word, const TUtf16String& pref) {
        return TWtringBuf{word, pref.size()} == pref;
    }

    static bool isPrefix(const TUtf16String& word, const TUtf16String& pref) {
        return word.StartsWith(pref);
    }

    static bool Eq(const TUtf16String& pref, const wchar16* word, size_t length) {
        return pref == TWtringBuf{word, length};
    }

    static bool Eq(const TUtf16String& s, const TUtf16String& word, size_t) {
        return s == word;
    }

    static bool Contains(const TUtf16String& s, wchar16 c, size_t = 0) {
        return s.Contains(c);
    }

    static bool Contains(const wchar16* s, wchar16 c, size_t length) {
        return TWtringBuf{s, length}.Contains(c);
    }

    void Replace(wchar16* word, size_t pos, wchar16 c) {
        word[pos] = c;
    }

    void Replace(TUtf16String& word, size_t pos, wchar16 c) {
        word[pos] = c;
    }

    template <typename T>
    void Replace(T& word, size_t pos, wchar16 c, NLemmer::TConvertRet& ret) {
        Replace(word, pos, c);
        ret.IsChanged.Set(NLemmer::CnvAdvancedConvert);
    }

    template<class T>
    void TRussianWordNormalizer::AdvancedConvert_(T& word, NLemmer::TConvertRet& ret) const {
        const TRussianConvertConsts* consts = Singleton<TRussianConvertConsts>();

        // fita-f, final er, etc.
        size_t len = ret.Length;

        for (size_t i = 0; i < len; ++i) {
            if (word[i] == consts->letYat) {                        // one,odne,odneh
                if (Eq(consts->strOne, word, len) || Eq(consts->strOdne, word, len) || Eq(consts->strOdneh, word, len)) {
                    Replace(word, i, consts->letI, ret);
                    break;
                } else {
                    Replace(word, i, consts->letE, ret);
                }
            }
            else if (word[i] == consts->letDecimalI && i < len - 1 &&       // Ukrainian single-dot
                        ((i > 0 && word[i - 1] == consts->letM && Contains(consts->setPp, word[i + 1])) ||  // мiръ
                         Contains(consts->setVocals, word[i + 1])))  // before vocal or semivocal
            {
                Replace(word, i, consts->letI, ret);
            }
        }
        //final (but not lonely) er
        if (len > 1 && word[len - 1] == consts->letHardSign && Contains(consts->setConsonants, word[len - 2]))
        {
            --len;
             ret.IsChanged.Set(NLemmer::CnvAdvancedConvert);
        }

        ret.Length = len;
        ret.Valid = ret.Valid && !Contains(word, consts->letDecimalI, ret.Length);

        if (ret.Length < 5)
            return;

        //prefixes - bes/bez, is/iz, etc.
        if (word[2] == consts->letZ) {
            if (isPrefix(word, consts->strVozs)    || isPrefix(word, consts->strNizs)
                || isPrefix(word, consts->strRazs) || isPrefix(word, consts->strRozs)
                || isPrefix(word, consts->strBe) && Contains(consts->setBreathConsonants, word[3]))
            {
                Replace(word, 2, consts->letS, ret);
            }
        } else if (word[2] == consts->letS) {
            if (isPrefix(word, consts->strIz)) {
                Replace(word, 1, consts->letS, ret);
            }
        } else if (ret.Length > 6){
            if (word[4] == consts->letZ){
                if (isPrefix(word, consts->strCherezs)) {
                    Replace(word, 4, consts->letS, ret);
                }
            } else if (word[4] == consts->letS) {
                if (isPrefix(word, consts->strChrez)) {
                    Replace(word, 3, consts->letS, ret);
                }
            }
        }
    }
} // NLemmer
} // NDetail

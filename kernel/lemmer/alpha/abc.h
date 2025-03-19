#pragma once

#include "diacritics.h"
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <library/cpp/enumbitset/enumbitset.h>
#include <library/cpp/langmask/langmask.h>

namespace NLemmer {
    namespace NDetail {
        class TTr;
    }
}

namespace NLemmer {
    class TAlphabet;
    class TAlphabetWordNormalizer;

    const TAlphabetWordNormalizer* GetAlphaRulesUnsafe(ELanguage id, bool primary=true);
    inline const TAlphabetWordNormalizer* GetAlphaRules(ELanguage id, bool primary=true) {
        const TAlphabetWordNormalizer* ret = GetAlphaRulesUnsafe(id, primary);
        if (!ret)
            ret = GetAlphaRulesUnsafe(LANG_UNK);
        Y_ASSERT(ret);
        return ret;
    }
    TLangMask ClassifyLanguageAlpha(const wchar16* word, size_t len, bool ignoreDigit, bool primary=true);
    TLangMask GetCharInfoAlpha(wchar16 ch, bool primary=true);
    TLangMask GetCharInfoAlphaRequired(wchar16 ch, bool primary=true);
    TCharCategory ClassifyCase(const wchar16* word, size_t len);

    enum EConvert {
        CnvDecompose,
        CnvCompose,
        CnvLowCase,
        CnvDerenyx,
        CnvConvert,
        CnvAdvancedConvert,
        CnvCheckCharacters,
        CnvMax,
        CnvUpCase = CnvMax,
        CnvTitleCase,
        CnvSoftDecompose,
        CnvSoftCompose,
        CnvMaxChanged
    };
    typedef TEnumBitSet<EConvert, CnvDecompose, CnvMax> TConvertMode;

    struct TConvertRet {
        bool Valid;
        size_t Length;
        TEnumBitSet<EConvert, CnvDecompose, CnvMaxChanged> IsChanged;

        explicit TConvertRet(size_t len, bool valid = true);
    };

    class TAlphabet {
    public:
        typedef unsigned char TCharClass;
        // + CharAlphaNormal - гласная (для бастардов)
        static const TCharClass CharAlphaRequired = 1;
        // обычная буква
        static const TCharClass CharAlphaNormal = 2;
        // буква получается в результате декомпозиции обычной буквы, но сама по себе в алфавит не входит
        static const TCharClass CharAlphaNormalDec = 4;
        // то же самое для ренухи
        static const TCharClass CharAlphaAlien = 8;
        static const TCharClass CharAlphaAlienDec = 16;
        // диакритика, по умолчанию - несущественная (ударение в русском)
        static const TCharClass CharDia = 32;
        // + CharDia диакритика существенная (умляут в немецком)
        static const TCharClass CharDiaSignificant = 64;
        // дефисы, апострофы
        static const TCharClass CharSign = 128;

    private:
        static_assert(sizeof(wchar16) == 2, "expect sizeof(wchar16) == 2");
        static const size_t MapSize = 65536;
        TCharClass Map[MapSize];

    public:
        class TComposer;
        THolder<TComposer> Composer;

    public:
        TAlphabet();
        TAlphabet(
            const wchar16* alphaRequired,
            const wchar16* alphaNormal,
            const wchar16* alphaAlien,
            const wchar16* diaAccidental,
            const wchar16* signs);
        ~TAlphabet();

        TCharClass CharClass(wchar16 c) const {
            return Map[c];
        }

        bool IsSignificant(wchar16 c) const {
            return (CharClass(c) & (CharDia | CharDiaSignificant)) != CharDia;
        }

    protected:
        void Create(
            const wchar16* alphaRequired,
            const wchar16* alphaNormal,
            const wchar16* alphaAlien,
            const wchar16* diaAccidental,
            const wchar16* signs);

    private:
        void AddAlphabet(const wchar16* arr, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed);
        void AddDecomposedChain(const wchar32* chain, TAlphabet::TCharClass cl, TAlphabet::TCharClass clDecomposed);
    };

    class TAlphabetWordNormalizer {
    private:
        class THelper;
        friend class THelper;

    private:
        const TAlphabet* Alphabet;
        // composition of prelower, lower and derenyx
        const NLemmer::NDetail::TTr* PreConverter;
        const NLemmer::NDetail::TTr* Derenyxer;
        const NLemmer::NDetail::TTr* Converter;
        const NLemmer::NDetail::TTr* PreLowerCaser;
        const NLemmer::NDetail::TTr* PreUpperCaser;
        const NLemmer::NDetail::TTr* PreTitleCaser;
        const NLemmer::TDiacriticsMap* DiacriticsMap;

    public:
        TAlphabetWordNormalizer(
            const TAlphabet* alphabet,
            const NLemmer::NDetail::TTr* preConverter,
            const NLemmer::NDetail::TTr* derenyxer,
            const NLemmer::NDetail::TTr* converter,
            const NLemmer::NDetail::TTr* preLowerCaser,
            const NLemmer::NDetail::TTr* preUpperCaser,
            const NLemmer::NDetail::TTr* preTitleCaser,
            const NLemmer::TDiacriticsMap* diacriticsMap = nullptr);

        TConvertRet Normalize(const wchar16* word, size_t length, wchar16* converted, size_t bufLen, TConvertMode mode = ~TConvertMode()) const;
        TConvertRet Normalize(TUtf16String& s, TConvertMode mode = ~TConvertMode()) const;

        TConvertRet PreConvert(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;

        TConvertRet ToLower(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet ToLower(TUtf16String& s) const;
        wchar16 ToLower(wchar16 a) const;

        TConvertRet ToUpper(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet ToUpper(TUtf16String& s) const;
        wchar16 ToUpper(wchar16 a) const;

        TConvertRet ToTitle(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet ToTitle(TUtf16String& s) const;
        wchar16 ToTitle(wchar16 a) const;

        TConvertRet Decompose(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet Decompose(TUtf16String& s) const;

        TConvertRet Compose(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet Compose(TUtf16String& s) const;

        TConvertRet SoftDecompose(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet SoftDecompose(TUtf16String& s) const;

        TConvertRet SoftCompose(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet SoftCompose(TUtf16String& s) const;

        TConvertRet Derenyx(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet Derenyx(TUtf16String& s) const;

        TConvertRet Convert(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet Convert(TUtf16String& s) const;

        TConvertRet AdvancedConvert(const wchar16* word, size_t length, wchar16* converted, size_t bufLen) const;
        TConvertRet AdvancedConvert(TUtf16String& s) const;

        const NLemmer::TDiacriticsMap* GetDiacriticsMap() const {
            return DiacriticsMap;
        }

        TAlphabet::TCharClass GetCharClass(const wchar16& alpha) const {
            return Alphabet ? Alphabet->CharClass(alpha) : TAlphabet::CharAlphaNormal;
        }

        virtual bool IsNormalized(const wchar16* word, size_t length) const;
        bool IsNormalized(const TUtf16String& s) const {
            return IsNormalized(s.data(), s.size());
        }

        static const TConvertMode& ModePreConvert() {
            static const TConvertMode mode(CnvCompose, CnvDerenyx);
            return mode;
        }

        static const TConvertMode& ModeConvertNormalized(bool skipAdvanced = false) {
            static const TConvertMode modeFull(CnvConvert, CnvAdvancedConvert, CnvCheckCharacters);
            static const TConvertMode modeSkipAdvanced(CnvConvert, CnvCheckCharacters);
            return skipAdvanced ? modeSkipAdvanced : modeFull;
        }

        virtual ~TAlphabetWordNormalizer(){};

    private:
        const TAlphabet* GetAlphabet() const {
            return Alphabet;
        }

    protected:
        virtual bool AdvancedConvert(const wchar16* word, size_t length, wchar16* converted, size_t bufLen, TConvertRet& ret) const;
        virtual bool AdvancedConvert(TUtf16String& s, TConvertRet& ret) const;
    };

    template <const wchar16* required, const wchar16* normal, const wchar16* alien, const wchar16* accidental, const wchar16* signs>
    class TTemplateAlphabet: public TAlphabet {
    public:
        TTemplateAlphabet()
            : TAlphabet(required, normal, alien, accidental, signs)
        {
        }
    };

    namespace NDetail {
        struct TTransdRet {
            size_t Length;
            size_t Processed;
            bool Valid;
            bool Changed;

            TTransdRet() {
            }

            TTransdRet(size_t len, size_t proc, bool val, bool changed = true)
                : Length(len)
                , Processed(proc)
                , Valid(val)
                , Changed(changed)
            {
            }
        };

        TTransdRet ComposeWord(const TAlphabet& alphabet, const wchar16* word, size_t length, wchar16* buffer, size_t bufSize);
        TTransdRet ComposeAlpha(const TAlphabet& alphabet, const wchar16* word, size_t length, wchar16* buffer, size_t bufSize);

        class TTr {
            typedef wchar16 TChr;
            static const size_t MapSize = 65536;
            static const TChr NeedAdv = 0xFFFF;

            TChr Map[MapSize];
            struct TAdvancedMap;
            const size_t AdvancedMapSize;
            THolder<TAdvancedMap, TDeleteArray> AdvancedMap;

        public:
            TTr(const TChr* from, const TChr* to, const TChr* kill, const TChr* fromAdv = nullptr, const TChr* const* toAdv = nullptr);
            size_t operator()(const TChr* src, TChr* dst) const;
            TTransdRet operator()(const TChr* src, size_t len, TChr* dst, size_t bufLen) const;

            TTransdRet operator()(TChr* s, size_t l) const {
                return (*this)(s, l, s, l);
            }
            size_t operator()(TChr* s) const {
                return (*this)(s, s);
            }
            ~TTr();

        private:
            const TChr* GetAdvDec(TChr c) const;
        };
    }
}

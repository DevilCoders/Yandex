#pragma once

/*
 * morfology module -- extern interface
 *
 * (c) comptek,iseg
 *
 *
 */

#include <cstring>
#include <cassert>

#include <util/generic/map.h>
#include <util/system/defaults.h>
#include <kernel/search_types/search_types.h>
#include <util/system/compat.h>
#include <util/generic/vector.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/charset/wide.h>

#include <library/cpp/langmask/langmask.h>
#include <kernel/lemmer/dictlib/grammar_enum.h>

const size_t MAX_GRAM_PER_WORD = 36;
const size_t MAXGRAM_BUF = 512;
const size_t MAXDISAMBIG_BUF = 512;
const size_t MAX_LEMMS = 28;
const size_t MAX_FORMS_PER_WORD = 2048;

namespace NLemmerAux {
    class TYandexLemmaSetter;
    class TYandexLemmaGetter;
    class TYandexWordformSetter;
}

namespace NLemmer {
    enum TLllLevel {
        LllNo = 0,
        Lll,
        LllSameText,
        LllTantum
    };

    class TFormGenerator;
    class TGrammarFiltr;
}

class TYandexLemma {
public:
    enum TQuality {
        QDictionary = 0x00000000,     // слово из словаря
        QBastard = 0x00000001,        // не словарное
        QSob = 0x00000002,            // из "быстрого словаря"
        QPrefixoid = 0x00000004,      // словарное + стандартный префикс (авто- мото- кино- фото-) всегда в компании с QBastard или QSob
        QFoundling = 0x00000008,      // непонятный набор букв, но проходящий в алфавит
        QBadRequest = 0x00000010,     // доп. флаг.: "плохая лемма" при наличии "хорошей" альтернативы ("махать" по форме "маша")
        QFromEnglish = 0x00010000,    // переведено с английского
        QToEnglish = 0x00020000,      // переведено на английский
        QUntranslit = 0x00040000,     // "переведено" с транслита
        QOverrode = 0x00100000,       // текст леммы был перезаписан
        QFix = 0x01000000,            // слово из фикс-листа
        QDisabled = 0x02000000,       // слово из отключенного языка (для флажка, отключающего морфологию в визарде)
        QAutomorphology = 0x04000000, // слово из автоморфологии
    };
    static const ui32 QAnyBastard = QBastard | QSob | QFoundling;
    static const ui32 AddGrammarSize = 3;
    static const ui32 InitialBufSize = MAXWORD_BUF * 2;

private:
    wchar16 LemmaText[MAXWORD_BUF];            // текст леммы (для пользователя)
    size_t LemmaLen;                         // длина леммы (strlen(LemmaText))
    size_t SuffixLen;                        // длина суффикса ("+" в "европа+")
    ELanguage Language;                      // код языка
    ui32 RuleId;                             // внутренний идентификатор типа парадигмы (почти никому не нужно)
    ui32 Quality;                            // см. TQuality
    ui32 Depth;                              // глубина сопоставления с основой при поиске бастарда (почти никому не нужно)
    const char* StemGram;                    // указатель на грамматику при основе (не зависящую от формы)
    const char* FlexGram[MAX_GRAM_PER_WORD]; // грамматики формы
    size_t GramNum;                          // размер массива грамматик формы
    size_t LemmaPrefLen;                     // длина приставки у леммы
    size_t PrefLen;                          // длина приставки
    size_t FlexLen;                          // длина окончания
    wchar16 FormInitial[InitialBufSize];       // текст анализируемой словоформы
    size_t FormInitialLen;                   // длина формы (strlen(FormInitial))
    wchar16 FormNormalized[MAXWORD_BUF];       // форма, приведённая к каноническому виду (диакритика, реникса...)
    size_t FormNormalizedLen;                // длина формы (strlen(FormNormalized))
    size_t FormConvertedLen;                 // длина формы, которая подавалась на вход движку (например, без апострофа в турецком)
    TCharCategory Flags;                     // флаги регистра символов - заглавные/прописные и т.п.
    size_t TokenPos;                         // [multitokens] смещение от начала слова (в токенах)
    size_t TokenSpan;                        // [multitokens] длина формы (в токенах)
    char AdditionGram[AddGrammarSize];       // грамматики, вычисленные во время нормализации (например, использовалась старая орфография)
    size_t Distortions[MAX_GRAM_PER_WORD];   // количество неточно совпавших символов
    size_t ParadigmId;                       // номер парадигмы: нужно для отсеивания повторяющихся бастардов в языках с диакритиками
    size_t MinDistortion;                    // минимальное количество искажений среди разборов формы
    double Weight;
    int LanguageVersion = -1;                // версия языка, которой сгенерирована лемма

public:
    bool Similar(const TYandexLemma& other) const {
        if (GetTextBuf() != other.GetTextBuf())
            return false;
        if (strcmp(GetStemGram(), other.GetStemGram()))
            return false;
        return ParadigmId == other.ParadigmId;
    }

    TYandexLemma();
    const wchar16* GetText() const {
        return LemmaText;
    }
    size_t GetTextLength() const {
        return LemmaLen;
    }
    TWtringBuf GetTextBuf() const {
        return {LemmaText, LemmaLen};
    }
    size_t GetSuffixLength() const {
        return SuffixLen;
    }
    ui32 GetDepth() const {
        return Depth;
    }
    ELanguage GetLanguage() const {
        return Language;
    }
    ui32 GetQuality() const {
        return Quality;
    }
    size_t GetLemmaPrefLen() const {
        return LemmaPrefLen;
    }
    size_t GetPrefLen() const {
        return PrefLen;
    }
    size_t GetFlexLen() const {
        return FlexLen;
    }
    const char* GetStemGram() const {
        return StemGram;
    }
    const char* GetAddGram() const {
        return AdditionGram;
    }
    const char* const* GetFlexGram() const {
        return FlexGram;
    }
    size_t FlexGramNum() const {
        return GramNum;
    }
    const wchar16* GetNormalizedForm() const {
        return FormNormalized;
    }
    size_t GetNormalizedFormLength() const {
        return FormNormalizedLen;
    }
    const wchar16* GetInitialForm() const {
        return FormInitial;
    }
    size_t GetInitialFormLength() const {
        return FormInitialLen;
    }
    size_t GetConvertedFormLength() const {
        return FormConvertedLen != 0 ? FormConvertedLen : FormNormalizedLen;
    }
    TCharCategory GetCaseFlags() const {
        return Flags;
    }
    size_t GetTokenPos() const {
        return TokenPos;
    }
    size_t GetTokenSpan() const {
        return TokenSpan;
    }
    bool IsBastard() const {
        return (GetQuality() & QAnyBastard) != 0;
    }
    bool IsBadRequest() const {
        return (GetQuality() & QBadRequest) != 0;
    }
    size_t GetParadigmId() const {
        return ParadigmId;
    }
    const size_t* GetDistortions() const {
        return Distortions;
    }
    size_t GetMinDistortion() const {
        return MinDistortion;
    }
    double GetWeight() const {
        return Weight;
    }
    int GetLanguageVersion() const {
        return LanguageVersion;
    }
    size_t GetFormsCount() const;

    bool HasStemGram(EGrammar gram) const;
    bool HasFlexGram(EGrammar gram) const;
    bool HasGram(EGrammar gram) const;

    NLemmer::TLllLevel LooksLikeLemma() const;
    TAutoPtr<NLemmer::TGrammarFiltr> LooksLikeLemmaExt() const;
    TAutoPtr<NLemmer::TFormGenerator> Generator(const NLemmer::TGrammarFiltr* filter = nullptr) const;

    friend class NLemmerAux::TYandexLemmaSetter;
    friend class NLemmerAux::TYandexLemmaGetter;
};

class TYandexWordform {
private:
    TUtf16String Text;
    ELanguage Language;
    size_t StemLen;
    size_t PrefixLen;
    size_t SuffixLen;
    const char* StemGram;
    TVector<const char*> FlexGram;
    double Weight;

public:
    TYandexWordform();
    const TUtf16String& GetText() const {
        return Text;
    }
    ELanguage GetLanguage() const {
        return Language;
    }
    size_t GetStemOff() const {
        return PrefixLen;
    }
    size_t GetStemLen() const {
        return StemLen;
    }
    size_t GetPrefixLen() const {
        return PrefixLen;
    }
    size_t GetSuffixLen() const {
        return SuffixLen;
    }
    const char* GetStemGram() const {
        return StemGram;
    }
    const char* const* GetFlexGram() const {
        return (FlexGram.size() > 0) ? &FlexGram[0] : nullptr;
    }
    size_t FlexGramNum() const {
        return FlexGram.size();
    }
    double GetWeight() const {
        return Weight;
    }
    friend class NLemmerAux::TYandexWordformSetter;
};

bool operator<(const TYandexLemma& l1, const TYandexLemma& l2);

bool operator<(const TYandexWordform& f1, const TYandexWordform& f2);

struct ConstCharComparer {
    bool operator()(const char* s1, const char* s2) const {
        if (!s1)
            return s2 != nullptr;
        if (!s2)
            return false;
        return strcmp(s1, s2) < 0;
    }
};

typedef TVector<TYandexLemma> TWLemmaArray;

typedef TVector<const TYandexLemma*> TWUniqueLemmaArray;

typedef TVector<TYandexWordform> TWordformArray;

size_t FillUniqLemmas(TWUniqueLemmaArray& uniques, const TYandexLemma data[], size_t len);

inline TLangMask GetLangMask(const TVector<const TYandexLemma*>& lemmas) {
    TLangMask r;
    for (size_t i = 0; i < lemmas.size(); ++i)
        r.SafeSet(lemmas[i]->GetLanguage());
    return r;
}

inline size_t FillUniqLemmas(TWUniqueLemmaArray& uniques, const TWLemmaArray& lemmaArray) {
    if (!lemmaArray.empty())
        return FillUniqLemmas(uniques, lemmaArray.data(), lemmaArray.size());
    else {
        uniques.clear();
        return uniques.size();
    }
}

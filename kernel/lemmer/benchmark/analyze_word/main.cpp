#include <library/cpp/testing/benchmark/bench.h>

#include <kernel/lemmer/core/language.h>
#include <util/random/random.h>
#include <util/random/mersenne.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/xrange.h>

using TWords = TVector<TUtf16String>;

enum class ERandomDictType {
    AddSuffix,
    AddPrefix
};

struct TEmpty {
    TWords Words;
    static const TEmpty& Instance() {
        return Default<TEmpty>();
    }

    TEmpty() {
        Words = {u""};
    }
};

template <ELanguage Lang>
struct TWordsHolder_Letters {
    TWords Words;
    static const TWordsHolder_Letters& Instance() {
        return Default<TWordsHolder_Letters<Lang>>();
    }

    TWordsHolder_Letters() {
        TVector<TString> Utf8Letters;
        switch (Lang) {
            case LANG_RUS: {
                Utf8Letters = {"а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", "р", "с", "т",
                               "у", "ф", "х", "ц", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я"};
                break;
            }
            case LANG_ENG: {
                Utf8Letters = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p", "q", "r", "s",
                               "u", "v", "w", "x", "y", "z"};
                break;
            }
            case LANG_KAZ: {
                Utf8Letters = {"а", "б", "в", "г", "д", "е", "ж", "з", "и", "й", "к", "л", "м", "н", "о", "п", "р", "с", "т",
                               "у", "ф", "х", "ц", "ш", "щ", "ъ", "ы", "ь", "э", "ю", "я", "ә", "і", "ң", "ғ", "ү", "ұ", "қ", "ө", "һ"};
                break;
            }
            default: {
                Y_FAIL("unknown language");
            }
        }
        for (auto l : Utf8Letters) {
            Words.push_back(UTF8ToWide(l));
        }
    }

    static ELanguage GetLang() {
        return Lang;
    }
};

template <ELanguage Lang>
struct TWordsHolder_Dictionary {
    TWords Words;
    static const TWordsHolder_Dictionary& Instance() {
        return Default<TWordsHolder_Dictionary<Lang>>();
    }

    TWordsHolder_Dictionary() {
        TVector<TString> Utf8Words;
        switch (Lang) {
            case LANG_ENG: {
                Utf8Words = {"bag", "lion", "cats", "animal", "dogs", "house", "computers", "summer", "winter", "telephones"};
                break;
            }
            case LANG_RUS: {
                Utf8Words = {"сумкой", "рюкзаком", "кошке", "собакам", "льву", "стол", "кресло", "магазине", "автомобиль", "телевизор"};
                break;
            }
            case LANG_KAZ: {
                Utf8Words = {"агротехникалық", "рақмет", "тоғыз", "кіші", "көгiлдiр", "қызыл", "жартысы", "тоқсан", "кешіріңіз", "менің"};
                break;
            }
            default: {
                Y_FAIL("unknown language");
            }
        }
        for (auto w : Utf8Words) {
            Words.push_back(UTF8ToWide(w));
        }
    }

    static ELanguage GetLang() {
        return Lang;
    }
};

template <ELanguage Lang>
struct TWordsHolder_Dict_Single_Long {
    TWords Words;
    static const TWordsHolder_Dict_Single_Long& Instance() {
        return Default<TWordsHolder_Dict_Single_Long<Lang>>();
    }

    TWordsHolder_Dict_Single_Long() {
        TVector<TString> Utf8Words;
        switch (Lang) {
            case LANG_ENG: {
                Utf8Words = {"Lopadotemachoselachogaleokranioleipsanodrimhypotrimmatosilphioparaomelitokatakechymenokichlepikossyphophattoperisteralektryonoptekephalliokigklopeleiolagoiosiraiobaphetraganopterygon"};
                break;
            }
            case LANG_RUS: {
                Utf8Words = {"Тетрагидропиранилциклопентилтетрагидропиридопиридиновые"};
                break;
            }
            case LANG_KAZ: {
                Utf8Words = {"қанағаттандырылмағандықтарыңыздан"};
                break;
            }
            default: {
                Y_FAIL("unknown language");
            }
        }
        for (auto w : Utf8Words) {
            Words.push_back(UTF8ToWide(w));
        }
    }

    static ELanguage GetLang() {
        return Lang;
    }
};

template <size_t Size, ERandomDictType Type, typename WordsDictType, ELanguage Lang>
struct TWordsHolder_Random {
    TWords Words;
    TMersenne<ui32> Generator;
    TWordsHolder_Random() {
        Generator = TMersenne<ui32>(53421);
        const TWords& dictWords = WordsDictType::Instance().Words;
        const TWords& letters = TWordsHolder_Letters<Lang>::Instance().Words;
        TUtf16String randomPart;
        Words.resize(dictWords.size());
        for (size_t i : xrange(dictWords.size())) {
            const TUtf16String& word = dictWords[i];
            randomPart.reserve(Size);
            for (size_t j : xrange(Size)) {
                Y_UNUSED(j);
                size_t randLetterIndex = Generator.Uniform(letters.size());
                randomPart += letters[randLetterIndex];
            }
            switch (Type) {
                case ERandomDictType::AddPrefix: {
                    Words[i] = randomPart + word;
                    break;
                }
                case ERandomDictType::AddSuffix: {
                    Words[i] = word + randomPart;
                    break;
                }
                default: {
                    Y_FAIL("unknown type");
                }
            }
            randomPart.clear();
        }
    }

    static ELanguage GetLang() {
        return Lang;
    }
};

#define DEFINE_BENCHMARK(Dict)                                                                                      \
    Y_CPU_BENCHMARK(AnalyzeWord_##Dict, iface) {                                                                    \
        const auto& dict = Default<Dict>();                                                                         \
        const TWords& words = dict.Words;                                                                           \
        for (size_t i = 0; i < iface.Iterations(); ++i) {                                                           \
            Y_UNUSED(i);                                                                                            \
            for (const TUtf16String& wideWord : words) {                                                            \
                TWLemmaArray out;                                                                                   \
                Y_DO_NOT_OPTIMIZE_AWAY(NLemmer::AnalyzeWord(wideWord.data(), wideWord.size(), out, TLangMask(dict.GetLang()))); \
            }                                                                                                       \
        }                                                                                                           \
    }

using TWordsHolder_Dict_LANG_ENG = TWordsHolder_Dictionary<LANG_ENG>;
using TWordsHolder_Dict_LANG_RUS = TWordsHolder_Dictionary<LANG_RUS>;
using TWordsHolder_Dict_LANG_KAZ = TWordsHolder_Dictionary<LANG_KAZ>;

DEFINE_BENCHMARK(TWordsHolder_Dict_LANG_ENG);
DEFINE_BENCHMARK(TWordsHolder_Dict_LANG_RUS);
DEFINE_BENCHMARK(TWordsHolder_Dict_LANG_KAZ);

using TWordsHolder_Random_AddSuffix_5_LANG_ENG = TWordsHolder_Random<5, ERandomDictType::AddSuffix, TWordsHolder_Dictionary<LANG_ENG>, LANG_ENG>;
using TWordsHolder_Random_AddSuffix_5_LANG_RUS = TWordsHolder_Random<5, ERandomDictType::AddSuffix, TWordsHolder_Dictionary<LANG_RUS>, LANG_RUS>;
using TWordsHolder_Random_AddSuffix_5_LANG_KAZ = TWordsHolder_Random<5, ERandomDictType::AddSuffix, TWordsHolder_Dictionary<LANG_KAZ>, LANG_KAZ>;
using TWordsHolder_Random_AddPrefix_5_LANG_ENG = TWordsHolder_Random<5, ERandomDictType::AddPrefix, TWordsHolder_Dictionary<LANG_ENG>, LANG_ENG>;
using TWordsHolder_Random_AddPrefix_5_LANG_RUS = TWordsHolder_Random<5, ERandomDictType::AddPrefix, TWordsHolder_Dictionary<LANG_RUS>, LANG_RUS>;
using TWordsHolder_Random_AddPrefix_5_LANG_KAZ = TWordsHolder_Random<5, ERandomDictType::AddPrefix, TWordsHolder_Dictionary<LANG_KAZ>, LANG_KAZ>;

DEFINE_BENCHMARK(TWordsHolder_Random_AddSuffix_5_LANG_ENG);
DEFINE_BENCHMARK(TWordsHolder_Random_AddSuffix_5_LANG_RUS);
DEFINE_BENCHMARK(TWordsHolder_Random_AddSuffix_5_LANG_KAZ);
DEFINE_BENCHMARK(TWordsHolder_Random_AddPrefix_5_LANG_ENG);
DEFINE_BENCHMARK(TWordsHolder_Random_AddPrefix_5_LANG_RUS);
DEFINE_BENCHMARK(TWordsHolder_Random_AddPrefix_5_LANG_KAZ);

using TWordsHolder_Letters_LANG_ENG = TWordsHolder_Letters<LANG_ENG>;
using TWordsHolder_Letters_LANG_RUS = TWordsHolder_Letters<LANG_RUS>;
using TWordsHolder_Letters_LANG_KAZ = TWordsHolder_Letters<LANG_KAZ>;

DEFINE_BENCHMARK(TWordsHolder_Letters_LANG_ENG);
DEFINE_BENCHMARK(TWordsHolder_Letters_LANG_RUS);
DEFINE_BENCHMARK(TWordsHolder_Letters_LANG_KAZ);

using TWordsHolder_Random_LANG_ENG = TWordsHolder_Random<50, ERandomDictType::AddPrefix, TEmpty, LANG_ENG>;
using TWordsHolder_Random_LANG_RUS = TWordsHolder_Random<50, ERandomDictType::AddPrefix, TEmpty, LANG_RUS>;
using TWordsHolder_Random_LANG_KAZ = TWordsHolder_Random<50, ERandomDictType::AddPrefix, TEmpty, LANG_KAZ>;

DEFINE_BENCHMARK(TWordsHolder_Random_LANG_ENG);
DEFINE_BENCHMARK(TWordsHolder_Random_LANG_RUS);
DEFINE_BENCHMARK(TWordsHolder_Random_LANG_KAZ);

using TWordsHolder_Dict_Single_Long_LANG_ENG = TWordsHolder_Dict_Single_Long<LANG_ENG>;
using TWordsHolder_Dict_Single_Long_LANG_RUS = TWordsHolder_Dict_Single_Long<LANG_RUS>;
using TWordsHolder_Dict_Single_Long_LANG_KAZ = TWordsHolder_Dict_Single_Long<LANG_KAZ>;

DEFINE_BENCHMARK(TWordsHolder_Dict_Single_Long_LANG_ENG);
DEFINE_BENCHMARK(TWordsHolder_Dict_Single_Long_LANG_RUS);
DEFINE_BENCHMARK(TWordsHolder_Dict_Single_Long_LANG_KAZ);

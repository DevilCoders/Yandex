#pragma once

/*
 * (C)++ iseg
 */

#include "grammar_enum.h"
#include "tgrammar_processing.h"

#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/singleton.h>
#include <util/system/defaults.h>

/*
проверка 2 списков характеристик на несовместимость
ПРЕДУСЛОВИЕ: они должны быть отсортированы
*/
int incompatible(char*, char*);

bool grammar_byk_from_rus(const char* grammar, char* outbuf, int bufsize);
// 14-09-96 uniq
int uniqsort(char* str, int len);

const int MAX_FORMA_NUM = 16000;
const int MIN_STEM_LEN = 2;
const int MIN_BASTARDED_WORD_LEN = 3;
const int MIN_BASTARDS_NOTSUBST_WORD_LEN = 5;

int common_grammar(TString gram[], int size, char* outgram);

TString sprint_grammar(const char* grammar, TGramClass hidden_classes = 0, bool russ = false);

using POS_SET = int;
inline POS_SET POS_BIT(unsigned char gPOS) {
    assert(gPOS > gBefore);
    assert(gPOS <= sizeof(POS_SET) * CHAR_BIT + gBefore);
    return 1 << (gPOS - (gBefore + 1));
}

extern const POS_SET Productive_PartsOfSpeech;
extern const POS_SET Preffixable_PartsOfSpeech;
extern const POS_SET ShortProductive_PartsOfSpeech;

// Mapping of grammeme codes to names and vice versa. The public interface
// includes only static methods for lookup and formatting of grammar info
class TGrammarIndex {
private:
    typedef THashMap<TStringBuf, EGrammar> TStrokaGrammarMap;
    typedef THashMap<TWtringBuf, EGrammar> TWtrokaGrammarMap;

    TStrokaGrammarMap CodeByNameIndex;
    TWtrokaGrammarMap WCodeByNameIndex;

    TVector<TString> YandexNameByCodeIndex;
    TVector<TString> Utf8NameByCodeIndex;
    TVector<TString> LatinNameByCodeIndex;

    TVector<TUtf16String> WNameByCodeIndex;
    TVector<TUtf16String> WLatinNameByCodeIndex;

    TVector<TGramClass> ClassByCodeIndex;

    typedef THashMap<TGramClass, TVector<EGrammar>> TClassToCodes;
    TClassToCodes CodesByClassIndex;

    // Singleton object to hold all indexes
    static const TGrammarIndex& TheIndex();

private:
    void RegisterGrammeme(EGrammar code, const char* name, const char* latin_name, TGramClass grClass);

    template <typename TStringType>
    inline static const TStringType& GetNameFromIndex(EGrammar code, const TVector<TStringType>& index) {
        size_t n = static_cast<size_t>(code);
        if (n >= index.size())
            return Default<TStringType>();
        return index[n];
    }

    template <typename TChr>
    inline static EGrammar GetCodeImpl(const TBasicStringBuf<TChr>& name, const THashMap<TBasicStringBuf<TChr>, EGrammar>& index) {
        if (name.empty())
            return gInvalid;

        typename THashMap<TBasicStringBuf<TChr>, EGrammar>::const_iterator it = index.find(name);
        if (it == index.end())
            return gInvalid;
        return (*it).second;
    }

public:
    // See constructor in ccl.cpp
    TGrammarIndex();

    inline static EGrammar GetCode(const TWtringBuf& name) {
        return GetCodeImpl(name, TheIndex().WCodeByNameIndex);
    }

    inline static EGrammar GetCode(const TStringBuf& name) {
        return GetCodeImpl(name, TheIndex().CodeByNameIndex);
    }

    inline static const char* GetName(EGrammar code) {
        return GetNameFromIndex(code, TheIndex().YandexNameByCodeIndex).c_str();
    }

    inline static const char* GetUtf8Name(EGrammar code) {
        return GetNameFromIndex(code, TheIndex().Utf8NameByCodeIndex).c_str();
    }

    inline static const char* GetLatinName(EGrammar code) {
        return GetNameFromIndex(code, TheIndex().LatinNameByCodeIndex).c_str();
    }

    inline static const wchar16* GetWName(EGrammar code) {
        return GetNameFromIndex(code, TheIndex().WNameByCodeIndex).c_str();
    }

    inline static const wchar16* GetWLatinName(EGrammar code) {
        return GetNameFromIndex(code, TheIndex().WLatinNameByCodeIndex).c_str();
    }

    inline static const TString& GetName(EGrammar code, bool latinic) {
        const TGrammarIndex& index(TheIndex());
        auto& table(latinic ? index.LatinNameByCodeIndex : index.YandexNameByCodeIndex);
        return GetNameFromIndex(code, table);
    }

    inline static TGramClass GetClass(EGrammar code) {
        size_t n = static_cast<size_t>(code);
        const TVector<TGramClass>& index = TheIndex().ClassByCodeIndex;

        if (n >= index.size())
            return 0;
        return index[n];
    }

    static TString& Format(TString& s, const EGrammar* grammar, const char* separator = ",") {
        s.remove();
        while (*grammar) {
            s += GetName(*grammar);
            grammar++;
            if (*grammar)
                s += separator;
        }
        return s;
    }

    static const TVector<EGrammar>& GetCodes(TGramClass grclass) {
        static TVector<EGrammar> empty;
        TClassToCodes::const_iterator i = TheIndex().CodesByClassIndex.find(grclass);
        if (i == TheIndex().CodesByClassIndex.end())
            return empty;
        return i->second;
    }
};

inline TGramClass GetClass(char c) {
    return TGrammarIndex::GetClass(NTGrammarProcessing::ch2tg(c));
}
inline void SkipClass(const char*& p, TGramClass cl) {
    while (GetClass(*p) & cl)
        p++;
}

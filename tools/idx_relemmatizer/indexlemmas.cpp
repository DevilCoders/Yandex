#include <kernel/search_types/search_types.h>
#include "indexconvert.h"

#include <library/cpp/token/charfilter.h>
#include <util/generic/singleton.h>

#include <stdlib.h>
#include <string.h>

namespace {
    template<ELanguage Lang>
    struct TIndexerOptBuilder {
        NLemmer::TAnalyzeWordOpt Res;
        TIndexerOptBuilder()
            : Res(NLemmer::TAnalyzeWordOpt::IndexerOpt())
        {
            Res.AcceptDictionary.Set(Lang);
            Res.AcceptSob.Set(Lang);
            Res.AcceptBastard.Set(Lang);
        }
    };
}; // anonymous namespace

struct TEmptyLemmatizer: public TLemmatizer {

    bool NeedLemmatizing(TKeyLemmaInfo&, const TStringBuf&, ELanguage) const override {
        return false;
    }

    bool NeedLemmatizing(TKeyLemmaInfo&, char [][MAXKEY_BUF], int) const override {
        return false;
    }

    void LemmatizeIndexWord(const TWtringBuf&, ELanguage, TVector<TWordLanguage>&, TWLemmaArray&) const override {
    }
};


template<ELanguage Lang>
struct TLangLemmatizer : public TLemmatizer {

    bool NeedLemmatizing(TKeyLemmaInfo &/*keyLemma*/, const TStringBuf& /*form*/, ELanguage lang) const override {
        return lang == Lang;
    }

    bool NeedLemmatizing(TKeyLemmaInfo &keyLemma, char forms[][MAXKEY_BUF], int formsCount) const override {
        if (keyLemma.Lang == Lang)
            return true;
        for (int i = 0; i != formsCount; ++i) {
            TStringBuf form(forms[i]);
            ELanguage lang = GetFormLang(form.data(), form.size());
            if (NeedLemmatizing(keyLemma, form, lang))
                return true;
        }
        return false;
    }

    void LemmatizeIndexWord(const TWtringBuf& word, ELanguage language, TVector<TWordLanguage>& res, TWLemmaArray& buffer) const override {
        NLemmer::AnalyzeWord(word.data(), word.size(), buffer, TLangMask(language), nullptr, Singleton<TIndexerOptBuilder<Lang> >()->Res);

        res.clear();
        res.resize(buffer.size());
        for (size_t i = 0; i < buffer.size(); ++i) {
            res[i].LemmaLen = NormalizeUnicode(buffer[i].GetText(), buffer[i].GetTextLength(), res[i].LemmaText, MAXWORD_BUF - 1);
            res[i].LemmaText[res[i].LemmaLen] = 0;
            res[i].Language = buffer[i].GetLanguage();
        }
    }
};

static void PrintUsageAndExit(const char* module) {
    Cerr << "Usage: " << module << " <tur|rus|eru|ukr|none> <src index name> <dst index name> [-v|--verbose]\n";
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc < 4)
        PrintUsageAndExit(argv[0]);
    try {
        bool verbose = false;
        if (argc == 5) {
            if (!strcmp(argv[4], "-v") || !strcmp(argv[4], "--verbose"))
                verbose = true;
            else
                PrintUsageAndExit(argv[0]);
        }
        if (!strcmp(argv[1], "tur")) {
            if (verbose)
                Cout << "Turkish convert\n";
            TLangLemmatizer<LANG_TUR> lemmatizer;
            return IndexConvert(argv[2], argv[3], lemmatizer, verbose);
        }
        if (!strcmp(argv[1], "eru")) {
            if (verbose)
                Cout << "Eng Rus Ukr convert\n";
            TLangLemmatizer<LANG_RUS> lemmatizer;
            return IndexConvert(argv[2], argv[3], lemmatizer, verbose);
        }
        if (!strcmp(argv[1], "rus")) {
            if (verbose)
                Cout << "Russian convert\n";
            TLangLemmatizer<LANG_RUS> lemmatizer;
            return IndexConvert(argv[2], argv[3], lemmatizer, verbose);
        }
        if (!strcmp(argv[1], "ukr")) {
            if (verbose)
                Cout << "Ukrainian convert\n";
            TLangLemmatizer<LANG_UKR> lemmatizer;
            return IndexConvert(argv[2], argv[3], lemmatizer, verbose);
        }
        if (!strcmp(argv[1], "none")) {
            if (verbose)
                Cout << "Empty convert\n";
            TEmptyLemmatizer lemmatizer;
            return IndexConvert(argv[2], argv[3], lemmatizer, verbose);
        }
        throw yexception() << "Language " << argv[1] << " is not tur|rus|none\n";
    } catch (const yexception&) {
        Cerr << "Exception caught:\n";
        Cerr << CurrentExceptionMessage() << "\n";
        return 1;
    }
    return 0;
}

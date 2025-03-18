#include "tokenizers.h"

#include <kernel/indexer/attrproc/url_transliterator.h>

#include <library/cpp/charset/wide.h>
#include <library/cpp/getopt/opt2.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/untranslit/untranslit.h>
#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/set.h>
#include <util/generic/string.h>

struct TOptions {
    class TInvalid: public yexception {
    };
    enum ETokenizer {
        TknSimple,
        TknUrl,
        TknNlp
    };
    enum EUseLemmer {
        LmrNoLemmer = 0,
        LmrNoDictionary = 1,
        LmrExactForms = 2,
        LmrLemmas = 3,
        LmrMax
    };
public:
    bool PrintUntranslit;
    bool PrintTranslit;
    bool PrintQuality;
    bool PrintText;
    size_t MaxNumOfTranslit;
    const TLanguage* Language;
    ECharset Encoding;
    ETokenizer Tokenizer;
    EUseLemmer Lemmer;
    bool TreatURLsAsIndexer;
    bool PrintTranslitWeight = 0;
    size_t MaxWeight = 0;
public:
    TOptions(int argc, char* argv[])
        : PrintUntranslit(true)
        , PrintTranslit(false)
        , PrintQuality(false)
        , PrintText(false)
        , MaxNumOfTranslit(9)
        , Language(nullptr)
        , Encoding(CODES_UNKNOWN)
        , Tokenizer(TknNlp)
        , Lemmer(LmrLemmas)
        , TreatURLsAsIndexer(false)
    {
        const char* lemmerUsageHelp =
        "- level of lemmer significance\n"
        "\t0 - just translit (no lemmer at all)\n"
        "\t1 - do not use lemmer's dictionaries\n"
        "\t2 - print exact forms\n"
        "\t3 - print lemmas\n\t";

        Opt2 opt(argc, argv, "tl:e:und:c:qwixm:");
        PrintTranslit = opt.Has('t', "- print translit");
        PrintQuality = opt.Has('q', "- print lemmer's quality");
        PrintText = opt.Has('w', "- print original word");
        MaxNumOfTranslit = opt.Int('c', "- max number of translit variants", 9);
        bool urlTkn = opt.Has('u', "- url tokenizer");
        bool smplTkn = opt.Has('n', "- simple tokenizer");
        Lemmer = static_cast<EUseLemmer>(opt.Int('d', lemmerUsageHelp, 2));
        const char* langName = opt.Arg('l', "- language", "rus");
        const char* encoding = opt.Arg('e', "- text encoding", "utf8");
        TreatURLsAsIndexer = opt.Has('i', "- treat URLs as indexer; print forms and lemmas from URL parts");
        PrintTranslitWeight = opt.Has('x', "- print untranslit weight");
        MaxWeight = opt.Int('m', "- max weight allowed", 0);

        opt.AutoUsageErr();

        Language = NLemmer::GetLanguageByName(langName);
        Encoding = CharsetByName(encoding);

        if (urlTkn)
            Tokenizer = TknUrl;
        else if (smplTkn)
            Tokenizer = TknSimple;
        if (PrintTranslit)
            PrintUntranslit = false;
        if (!Language)
            ythrow TInvalid() << "Unknown language \"" << langName;
        if (Encoding == CODES_UNKNOWN)
            ythrow TInvalid() << "Unrecognized encoding: \"" << encoding << "\"";
        if (urlTkn && smplTkn)
            ythrow TInvalid() << "Can't use url and simple tokenizer simultaneously";
        if (Lemmer >= LmrMax)
            ythrow TInvalid() << "Invalid level " << static_cast<size_t>(Lemmer);
    }
};

class TTranslitTokenHandler: public ITokenHandler {
private:
    const TOptions& Opt;
public:
    TTranslitTokenHandler(const TOptions& opt)
        : Opt (opt)
    {}
private:
    void OnToken(const TWideToken& token, size_t origleng, NLP_TYPE type) override {
        if (!IsGood(type))
            return;
        TUtf16String word(token.Token, origleng);
        OnWord(word);
    }

    static bool IsGood(NLP_TYPE type) {
        switch (type) {
        case NLP_WORD:
        case NLP_INTEGER:
        case NLP_FLOAT:
        case NLP_MARK:
            return true;
        default:
            return false;
        }
    }

    void OnWord(const TUtf16String& word) const {
        if (Opt.PrintText)
            Cout << word << " ";
        if (Opt.PrintUntranslit) {
            if (Opt.Lemmer == TOptions::LmrNoLemmer)
                PrintNoLemmerTranslit(Opt.Language->GetUntransliter(word).Get());
            else
                PrintLemmerTranslit(word);
        }
        if (Opt.PrintTranslit)
            PrintNoLemmerTranslit(Opt.Language->GetTransliter(word).Get());
        Cout << Endl;
    }

    void PrintNoLemmerTranslit(TUntransliter* trn) const {
        TUntransliter::WordPart wp = trn->GetNextAnswer();
        if (!wp.Empty()) {
            Cout << WideToChar(wp.GetWord(), Opt.Encoding);
            if (Opt.PrintTranslitWeight) {
                Cout << ' ' << wp.Quality();
            }
            size_t qmax = (size_t)((wp.Quality() ? wp.Quality() : 1) * TUntransliter::DefaultQualityRange + 0.5);
            if (Opt.MaxWeight) {
                qmax = Opt.MaxWeight;
            }
            for (size_t cnt = 1; cnt < Opt.MaxNumOfTranslit; ++cnt) {
                wp = trn->GetNextAnswer(qmax);
                if (wp.Empty())
                    break;
                Cout << " " << WideToChar(wp.GetWord(), Opt.Encoding);
                if (Opt.PrintTranslitWeight) {
                    Cout << ' ' << wp.Quality();
                }
            }
        }
        Cout << Endl;
    }

    static void PrintQuality(ui32 qual) {
        if (qual & TYandexLemma::QBastard) {
            Cout << "(B";
            if (qual & TYandexLemma::QPrefixoid)
                Cout << "P";
            Cout << ")";
        } else if (qual & TYandexLemma::QSob) {
            Cout << "(S";
            if (qual & TYandexLemma::QPrefixoid)
                Cout << "P";
            Cout << ")";
        } else if (qual & TYandexLemma::QFoundling) {
            Cout << "(F)";
        }
    }

    void PrintLemmerTranslit(const TUtf16String& word) const {
        NLemmer::TTranslitOpt* translitOpt = nullptr;
        NLemmer::TTranslitOpt translitOptObj;
        if (Opt.Lemmer != TOptions::LmrNoDictionary) {
            translitOptObj = ChooseOpt(Opt.Lemmer);
            translitOpt = &translitOptObj;
        }

        TWLemmaArray arr;
        Opt.Language->FromTranslit(word.c_str(), word.length(), arr, size_t(-1), translitOpt);
        TSet<TUtf16String> out;
        for (size_t i = 0; i < arr.size() && out.size() < Opt.MaxNumOfTranslit; ++i) {
            TUtf16String s(arr[i].GetText(), arr[i].GetTextLength());
            if (out.find(s) != out.end())
                continue;
            if (out.size())
                Cout << " ";
            Cout << WideToChar(s, Opt.Encoding);
            if (Opt.PrintQuality)
                PrintQuality(arr[i].GetQuality());
            out.insert(s);
        }
        Cout << Endl;
    }

    static NLemmer::TTranslitOpt ChooseOpt(TOptions::EUseLemmer lm) {
        switch (lm) {
        case TOptions::LmrNoDictionary:
        default:
            Y_ASSERT(false);
            return NLemmer::TTranslitOpt();
        case TOptions::LmrExactForms: {
            NLemmer::TTranslitOpt opt;
            opt.ExactForms = ~NLemmer::TLanguageQualityAccept();
            return opt;
        }
        case TOptions::LmrLemmas:
            return NLemmer::TTranslitOpt();
        }
    }
};

void ProcessUrl(const TString& url, ELanguage lang = LANG_RUS) {
    TUrlTransliterator::TTransliteratorCache cache;
    TUrlTransliterator transliterator(cache, url, lang, nullptr);
    while (transliterator.Has()) {
        TUrlTransliteratorItem item;
        transliterator.Next(item);
        const ui32 wordIndex = item.GetIndex() + TWordPosition::FIRST_CHILD;
        if (wordIndex >= (1 << WORD_LEVEL_Bits))
            break;
        Cout << item.GetForma() << "\t" << item.GetLowerCaseLemma() << Endl;
    }
}

TAutoPtr<ITranslitTokenizer> GetTokenizer(const TOptions& opt, TTranslitTokenHandler& handler) {
    switch (opt.Tokenizer) {
    case TOptions::TknSimple:
        return new TSimpleTranslitTokenizer(handler);
    case TOptions::TknUrl:
        return new TUrlTranslitTokenizer(handler);
    case TOptions::TknNlp:
        return new TNlpTranslitTokenizer(handler);
    }
    return nullptr;
}

int main(int argc, char* argv[])
{
    try {
        const TOptions opt(argc, argv);

        TString word;
        while (Cin.ReadLine(word)) {
            if (opt.TreatURLsAsIndexer) {
                ProcessUrl(word, opt.Language->Id);
            } else {
                TAutoPtr<TTranslitTokenHandler> handler = new TTranslitTokenHandler(opt);
                TAutoPtr<ITranslitTokenizer> tokenizer = GetTokenizer(opt, *handler);
                tokenizer->Tokenize(CharToWide(word, opt.Encoding));
            }
        }
    } catch (const TOptions::TInvalid& e) {
        Cerr << e.what() << Endl;
        return 1;
    }
    return 0;
}

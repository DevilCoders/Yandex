#include <kernel/search_types/search_types.h>
#include <kernel/lemmer/automorphology/automorphology.h>
#include <dict/disamb/mx_disamb/disamb.h>

#include <library/cpp/getopt/last_getopt.h>
#include <kernel/lemmer/automorphology/automorphology.h>
#include <kernel/lemmer/core/handler.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/wordinstance.h>
#include <kernel/lemmer/core/decimator.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <kernel/lemmer/dictlib/grammar_index.h>
#include <kernel/lemmer/fixlist_load/from_text.h>
#include <kernel/lemmer/tools/handler.h>
#include <kernel/lemmer/tools/order.h>
#include <dict/disamb/zel_disamb/zel_disamb_handler.h>
#include <library/cpp/svnversion/svnversion.h>
#include <library/cpp/token/charfilter.h>
#include <library/cpp/langmask/index_language_chooser.h>
#include <library/cpp/tokenizer/tokenizer.h>

#include <library/cpp/charset/doccodes.h>
#include <library/cpp/langs/langs.h>
#include <library/cpp/charset/recyr.hh>
#include <library/cpp/charset/wide.h>
#include <util/generic/algorithm.h>
#include <util/generic/map.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/file.h>
#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/yassert.h>

using NLemmer::TLemmerResult;
using NLemmer::TLemmerResults;
using NLemmer::ILemmerResultsHandler;

struct TPrintParams {
    ECharset Encoding;
    bool PrintParadigm;
    bool EnglishGrammar;
    bool DecomposeLemma;
    int Verbosity;
    bool PrintWeight;
    bool PrintContextWeight;

    TMap<TCharCategory, TString> CaseFlagNames;

    TPrintParams()
        : Encoding(CODES_UTF8)
        , PrintParadigm(false)
        , EnglishGrammar(true)
        , DecomposeLemma(false)
        , Verbosity(2)
        , PrintWeight(false)
        , PrintContextWeight(false)
    {
        CaseFlagNames[CC_ALPHA] = "ALPHA";
        CaseFlagNames[CC_NMTOKEN] = "NMTOKEN";
        CaseFlagNames[CC_NUMBER] = "NUMBER";
        CaseFlagNames[CC_NUTOKEN] = "NUTOKEN";
        CaseFlagNames[CC_ASCII] = "ASCII";
        CaseFlagNames[CC_NONASCII] = "NONASCII";
        CaseFlagNames[CC_TITLECASE] = "TITLECASE";
        CaseFlagNames[CC_UPPERCASE] = "UPPERCASE";
        CaseFlagNames[CC_LOWERCASE] = "LOWERCASE";
        CaseFlagNames[CC_MIXEDCASE] = "MIXEDCASE";
        CaseFlagNames[CC_COMPOUND] = "COMPOUND";
        CaseFlagNames[CC_HAS_DIACRITIC] = "HAS_DIACRITIC";
        CaseFlagNames[CC_DIFFERENT_ALPHABET] = "DIFFERENT_ALPHABET";
    }
};

struct TParams {
    TString InFile;
    TString OutFile;
    TLangMask LanguagesMask;
    TVector<ELanguage> LanguagesList;
    TWordFilter StopWords;

    bool CompReadable;
    bool TestingOutput;
    bool WordInstance;
    bool PrintLine;
    bool TryUtfInput;
    bool AllBastards;
    bool GenerateQuasiBastards;
    bool GenerateAllBastards;
    ui16 LastNonRareRuleId;
    bool RobotOptions;
    bool Trans;
    char GrammarBuf[MAXGRAM_BUF];
    TPrintParams PrintParams;
    TString Disambiguate;
    bool Spellcheck;
    bool UseOldMarkTokenization;
    TString DecimatorConfig;
    bool SkipTokenization;
    bool DoNotPrune;
    bool ListerOutput;
    bool SkipNormalization;

    TParams ()
        : LanguagesMask(LI_ALL_LANGUAGES)
        , CompReadable(false)
        , TestingOutput(false)
        , WordInstance(false)
        , PrintLine(false)
        , TryUtfInput(false)
        , AllBastards(false)
        , GenerateQuasiBastards(false)
        , GenerateAllBastards(false)
        , LastNonRareRuleId(0)
        , RobotOptions(false)
        , Trans(false)
        , Disambiguate()
        , Spellcheck(false)
        , UseOldMarkTokenization(false)
        , SkipTokenization(false)
        , DoNotPrune(false)
        , ListerOutput(false)
        , SkipNormalization(false)
    {
        LanguagesList.push_back(LANG_UNK);
        GrammarBuf[0] = 0;
    }

    void SetLangMask(const char* list) {
        LanguagesMask.Reset();
        TVector<ELanguage> items;
        ParseLanguageList(list, items);
        TVector<ELanguage>::const_iterator it = items.begin();
        while (it != items.end())
            LanguagesMask.SafeSet(*(it++));
    }

    void SetLangList(const char* list) {
        LanguagesList.clear();
        ParseLanguageList(list, LanguagesList);
        LanguagesList.push_back(LANG_UNK);
    }

    void SetStopWordList(const char* fname) {
        StopWords.InitStopWordsList(fname);
    }

    void SetAcceptedGrammemes(const char *list) {
        grammar_byk_from_rus(list, GrammarBuf, MAXGRAM_BUF);
    }

    void SetRobotLanguage(ELanguage lang) {
        TIndexLanguageOptions opt(lang);
        if (LanguagesMask == LI_ALL_LANGUAGES)
            LanguagesMask = opt.GetLangMask();

        if (LanguagesList.size() <= 1) {
            LanguagesList.clear();
            for (size_t i = 0; opt.GetLangPriorityList()[i]; ++i)
                LanguagesList.push_back(opt.GetLangPriorityList()[i]);
            LanguagesList.push_back(LANG_UNK);
        }
    }

    static void ParseLanguageList(const char* list, TVector<ELanguage>& parsed) {
        static const char* language_separators = ",:; ";
        while (*list) {
            list += strspn(list, language_separators);
            if (!*list)
                break;
            size_t len = strcspn(list, language_separators);

            TString langname(list, 0, len);
            ELanguage lang = LanguageByNameStrict(langname.c_str());
            if (lang != LANG_MAX)
                parsed.push_back(lang);
            else
                Cerr << "Unrecognized language name " << langname << " - ignored" << Endl;
            list += len;
        }
    }
};

void PrintTesting(IOutputStream& out, const TLemmerResults& results, const TPrintParams& params);
void PrintComputerReadable(IOutputStream& out, const TLemmerResults& results, const TPrintParams& params);
void PrintHumanReadable(IOutputStream& out, const TLemmerResults& results, const TPrintParams& params);
void PrintLister(IOutputStream& out, const TLemmerResults& results);

void PrintWordInstanceAnalyses(IOutputStream& out, const TWideToken& tok, const TLanguageContext& langContext, bool printParadigm = false);

static void printFingerprints(const TLangMask& langs) {
    Cout << "Languages fingerprints:\n";

    const NLemmer::TLanguageVector& langlist = NLemmer::GetLanguageList();
    NLemmer::TLanguageVector::const_iterator it = langlist.begin();
    while (it != langlist.end()) {
        const TLanguage *lang = *(it++);
        if (lang->Id == LANG_UNK || langs.Test(lang->Id))
            Cout << "    " << lang->Code() << ": " << lang->DictionaryFingerprint() << Endl;
    }
}

class TLemmerTokenHandler : public ILemmerResultsHandler {
private:
    enum TOut {
        outNull,
        outTest,
        outComp,
        outNorm,
        outWordInstance,
        outLister
    };

private:
    IOutputStream& Out;
    TOut OutType;
    TLanguageContext LangContext;
    size_t NumTokens;
    NLemmer::TAnalyzeWordOpt AnalyzeOpt;
    TPrintParams PrintParams;
    TFormDecimator Decimator;

public:
    TLemmerTokenHandler(const TParams& params, IOutputStream& out)
        : Out(out)
        , OutType(outNorm)
        , LangContext(params.LanguagesMask, params.LanguagesList.data(), params.StopWords)
        , NumTokens(0)
        , AnalyzeOpt(params.RobotOptions ? NLemmer::TAnalyzeWordOpt::IndexerOpt() :
                    NLemmer::TAnalyzeWordOpt::DefaultLemmerTestOpt())
        , PrintParams(params.PrintParams)
    {
        if (params.DecimatorConfig)
            Decimator.InitDecimator(params.DecimatorConfig.data());
        else
            Decimator = DefaultFormDecimator();
        LangContext.SetDecimator(Decimator);
        if (PrintParams.Verbosity < 0)
            OutType = outNull;
        else if (params.WordInstance)
            OutType = outWordInstance;
        else if (params.TestingOutput)
            OutType = outTest;
        else if (params.CompReadable)
            OutType = outComp;
        else if (params.ListerOutput)
            OutType = outLister;

        if (!params.AllBastards)
            AnalyzeOpt.AcceptBastard &= ~NLanguageMasks::NoBastardsInSearch();
        if (params.GenerateQuasiBastards)
            AnalyzeOpt.GenerateQuasiBastards = ~TLangMask();
        else
            AnalyzeOpt.GenerateQuasiBastards = TLangMask();
        AnalyzeOpt.NeededGramm = params.GrammarBuf;
        if (params.Trans) {
            AnalyzeOpt.AcceptFromEnglish = ~TLangMask();
            AnalyzeOpt.AcceptTranslit = ~TLangMask();
        }
        if (params.GenerateAllBastards)
            AnalyzeOpt.GenerateAllBastards = params.GenerateAllBastards;
        if (params.LastNonRareRuleId != 0)
            AnalyzeOpt.LastNonRareRuleId = params.LastNonRareRuleId;
        if (params.DoNotPrune)
            AnalyzeOpt.DoNotPrune = params.DoNotPrune;

        if (params.SkipNormalization)
            AnalyzeOpt.SkipNormalization = true;
    }

    bool OnLemmerResults(const TWideToken& tok, NLP_TYPE type, const TLemmerResults* results) override {
        if (type == NLP_MISCTEXT || GetSpaceType(type) != ST_NOBRK)
            return false;
        ++NumTokens;

        const TWideToken* pTok = &tok;

        TWideToken temp (tok.Token, tok.Leng);
        if (tok.SubTokens.size() == 0
            || (tok.SubTokens.size() == 1 && tok.SubTokens[0].Len == 0))
        {
            pTok = &temp;
        }

        if (OutType == outWordInstance) {
            PrintWordInstanceAnalyses(Out, *pTok, LangContext, PrintParams.PrintParadigm);
        } else if (results) {
            PrintLemmerResults(tok, results);
        }

        return true;
    }

    void PrintLemmerResults(const TWideToken& token, const TLemmerResults* results) {
        if (results == nullptr) {
            return;
        }

        TLemmerResults sortedRes = *results;
        StableSort(sortedRes.begin(), sortedRes.end(), NLemmer::TLemmerResultOrderGreater());

        switch (OutType) {
                case outNull:
                    break;
                case outTest:
                    PrintTesting(Out, sortedRes, PrintParams);
                    break;
                case outComp:
                    PrintComputerReadable(Out, sortedRes, PrintParams);
                    break;
                case outLister:
                    PrintLister(Out, sortedRes);
                    break;
                case outNorm:
                default:
                    int count = sortedRes.size();
                    Out << WideToChar(token.Token, token.Leng, PrintParams.Encoding)
                        << ": " << count << " solution" << (count == 1?"" : "s") << " found" << "\n\n";
                    PrintHumanReadable(Out, sortedRes, PrintParams);
                    break;
        }
    }

    void Flush() override {
    }

    void ResetNumToken() {
        NumTokens = 0;
    }
    size_t GetNumTokens() const {
        return NumTokens;
    }
    const TLanguageContext& GetLanguageContext() const {
        return LangContext;
    }
    const NLemmer::TAnalyzeWordOpt& GetAnalyzeOpt() const {
        return AnalyzeOpt;
    }
};

void PrintWordInstanceAnalyses(IOutputStream& out, const TWideToken& tok, const TLanguageContext& langContext, bool printParadigm) {
    size_t startPos = 0;
    size_t endPos = tok.Leng;
    TWordInstance wi;
    wi.Init(TUtf16String(tok.Token, tok.Leng), TCharSpan(0, tok.Leng), langContext);
    TWordInstanceUpdate(wi).ShrinkIntrusiveBastards();
    TWordInstanceUpdate(wi).RemoveBadRequest();

    for (TWordInstance::TLemmasVector::const_iterator i = wi.GetLemmas().begin(), mi = wi.GetLemmas().end(); i != mi; ++i) {
        out << startPos << " "
            << endPos << " "
            << i->GetLemma() << " ";

        if (i->GetLanguage())
            out << IsoNameByLanguage(i->GetLanguage());
        out << " ";

        out << " "
            << i->GetStemGrammar().ToString() << " "
            << i->IsBastard() << " "
            << i->IsStopWord() << " "
            << "\n";

        if (printParadigm) {
           TVector<TUtf16String> forms;
           for (const auto& formAndWeight : i->GetForms())
               forms.push_back(formAndWeight.first);
           Sort(forms.begin(), forms.end());
           for (const auto& form : forms) {
               out << "  " << form << "\t";
               bool firstGram = true;
               for (const auto& grams : i->GetForms().find(form)->second.Grams) {
                   if (!firstGram)
                       out << " | ";
                   firstGram = false;
                   TGramBitSet flexGrams = grams & ~i->GetStemGrammar();
                   flexGrams.ToString(out, ",");
               }
               out << "\n";
           }
        }
    }
    out << Endl;
}

void ProcessFile(IInputStream& in, IOutputStream& out, const TParams& params) {

    TLemmerTokenHandler handler(params, out);

    NLemmer::TLemmerHandler::TParams lemmerParams(handler.GetLanguageContext().GetLangMask()
            , handler.GetLanguageContext().GetLangOrder()
            , handler.GetAnalyzeOpt());


    TAutoPtr<ILemmerResultsHandler> disambHandler;

    TAutoPtr<NLemmer::TLemmerHandler> lemmerHandler;
    if (!params.Disambiguate.empty()) {
        if (params.Disambiguate == "zel") {
            disambHandler.Reset(new TZelDisambHandler(handler));
        }
        if (params.Disambiguate == "mx") {
            disambHandler.Reset(new NMxNetDisamb::TDisambModel(handler));
        }
        Y_VERIFY(disambHandler.Get() && "Unset disamb handler");
        lemmerHandler.Reset(new NLemmer::TLemmerHandler(lemmerParams, *disambHandler.Get()));
    } else {
        lemmerHandler.Reset(new NLemmer::TLemmerHandler(lemmerParams, handler));
    }

    TNlpTokenizer tokenizer(*lemmerHandler.Get(), params.UseOldMarkTokenization);

    TString line;
    while (in.ReadLine(line)) {
        if(params.PrintLine)
            out << line << "\n" << "\n";
        TUtf16String txt;
        txt.resize(line.length());
        size_t w = 0;
        if (params.TryUtfInput && UTF8ToWide(line.c_str(), line.size(), txt.begin(), w))
            txt.resize(w);
        else
            txt = CharToWide(line, params.PrintParams.Encoding);
        handler.ResetNumToken();

        if (params.Spellcheck) {
            bool first = true;
            for (int lang = LANG_UNK; lang != LANG_MAX; ++lang) {
                if (params.LanguagesMask.SafeTest((ELanguage)lang)) {
                    const TLanguage* language = NLemmer::GetLanguageById((ELanguage)lang);
                    if (language != nullptr) {
                        if (params.CompReadable) {
                            if (!first) {
                                out << "|";
                            }
                            else {
                                out << txt << " ";
                            }
                            out << language->Code() << " " << language->Spellcheck(txt.data(), txt.size());
                            first = false;
                        }
                        else {
                            out << "Spellcheck for " << language->Code() << ": " << language->Spellcheck(txt.data(), txt.size()) << Endl;
                        }
                    }
                }
            }
            if (params.CompReadable) {
                out << Endl;
            }
            continue;
        }

        if (!params.SkipTokenization) {
            tokenizer.Tokenize(txt.data(), txt.size());
        } else {
            lemmerHandler->OnToken(TWideToken(txt.data(), txt.size()), txt.size(), NLP_WORD);
        }
        lemmerHandler->Flush();
        if (!handler.GetNumTokens())
            out << Endl;

        if(params.PrintLine)
            out << "====" << Endl;
    }
}

TString RecodeGrammarFromYandex(ECharset encoding, const char* gram, bool english) {
    return RecodeFromYandex(encoding, sprint_grammar(gram, 0, !english));
}

inline TString RecodeFromTChar(const TUtf16String& s, ECharset encoding) {
    return WideToChar(s.c_str(), s.size(), encoding);
}

inline TString RecodeFromTChar(ECharset encoding, const TChar* s, size_t len) {
    return WideToChar(s, len, encoding);
}


TString PrintParadigm(ECharset encoding, const TYandexLemma& lemma, const char* delim, bool englishGrammar, bool printWeights = false) {
    TString out;
    TWordformArray forms;
    size_t numForms = NLemmer::Generate(lemma, forms);
    ui32 total = 1;
    if (printWeights) {
        total = 0;
        for (size_t j = 0; j != numForms; ++j)
            total += forms[j].GetWeight();
        if (total == 0)
            total = 1;
    }
    for (size_t j = 0; j != numForms; ++j) {
        if (j)
            out += delim;

        if (printWeights) {
            out += Sprintf("%.6lf", double(forms[j].GetWeight()) / total);
            out += " ";
        }
        const TUtf16String& text = forms[j].GetText();
        size_t stemBegin = forms[j].GetStemOff();
        size_t stemEnd   = forms[j].GetStemOff() + forms[j].GetStemLen();
        TString pref = RecodeFromTChar(TUtf16String(text.begin(), text.begin() + stemBegin), encoding);
        TString stem = RecodeFromTChar(TUtf16String(text.begin() + stemBegin, text.begin() + stemEnd), encoding);
        TString suff = RecodeFromTChar(TUtf16String(text.begin() + stemEnd, text.end()), encoding);

        out += pref;
        out += "[";
        out += stem;
        out += "]";
        out += suff;
        out += "{";
        for (size_t k = 0; k < forms[j].FlexGramNum(); ++k) {
            if (k)
                out += "|";
            out += RecodeGrammarFromYandex(encoding, forms[j].GetFlexGram()[k], englishGrammar);
        }
        out += "}";
    }
    return out;
}

static TString SubstGramm(const char* minuend, const char* subtrahend) {
    TString r;
    for (; *minuend; ++minuend) {
        bool fl = true;
        for (; *subtrahend && fl; ++subtrahend)
            fl = *subtrahend != *minuend;
        if (fl)
            r += minuend;
    }
    return r;
}

TString ComputerReadable(ECharset encoding, const TLemmerResult& result,
                           TMap<TCharCategory, TString> case_flag_names,
                           bool printParadigm, bool printRuleId, bool englishGrammar, bool decomposeLemma, bool printWeight)
{
    TString out;
//    Выводим следующие:
//    char   text[MAXWORD_BUF];                 // текст леммы
//    unsigned rule_id;                         // id правила

//    size_t quality;                           // словарное/несловарное, плохой разбор, etc.
//    const char *stem_gram;                    // указатель на грамматику при основе (не зависящую от формы)
//    const char *flex_gram[MAX_GRAM_PER_WORD]; // грамматики формы - список вариантов разделённых "|"

//    char Form[MAXWORD_BUF];                   // нормализованный текст анализируемой словоформы
//    int TokenPos;                             // [multitokens] смещение от начала слова (в токенах)
//    int TokenSpan;                            // [multitokens] длина формы (в токенах)
//    ELanguage Language;                     // код языка
//    TCharCategory Flags;                      // флаги регистра символов - заглавные/прописные и т.п.
//    float Weight;                             // вес леммы
//
    const TYandexLemma& lemma = result.first;

    if (decomposeLemma)
        out += RecodeFromTChar(NormalizeUnicode(TUtf16String(lemma.GetText(), lemma.GetTextLength())), encoding);
    else
        out += RecodeFromTChar(encoding, lemma.GetText(), lemma.GetTextLength());
    out += " ";
    if (printRuleId) {
        out += ToString(NLemmerAux::GetRuleId(lemma));
        out += " ";
    }

    {
        bool fl = false;
        if ((lemma.GetQuality() & TYandexLemma::QBastard) && !(lemma.GetQuality() & TYandexLemma::QPrefixoid)) {
            out += "Bastard";
            fl = true;
        }
        if (lemma.GetQuality() & TYandexLemma::QFoundling) {
            out += "Foundling";
            fl = true;
        }
        if (lemma.GetQuality() & TYandexLemma::QSob) {
            out += "Sob";
            fl = true;
        }
        if (lemma.GetQuality() & TYandexLemma::QFix) {
            out += "Fix";
            fl = true;
        }
        if (lemma.GetQuality() & TYandexLemma::QPrefixoid) {
            if (fl)
                out += "|";
            out += "Prefixoid";
            fl = true;
        }
        if (!fl)
            out += "Good";
        if (lemma.GetQuality() & TYandexLemma::QBadRequest) {
            out += "|";
            out += "BadRequest";
        }
        if (lemma.LooksLikeLemma()) {
            out += "|";
            out += "LLL";
        }
        out += " ";
    }

    out += RecodeGrammarFromYandex(encoding, lemma.GetStemGram(), englishGrammar);
    out += " ";
    //out << lemma.gram_num << " ";
    //out << lemma.flex_gram << " ";

    TString temp = "";
    const TString addGr = SubstGramm(lemma.GetAddGram(), lemma.GetStemGram());
    for (size_t k = 0; k < lemma.FlexGramNum(); k++) {
        if (!temp.empty())
            temp += "|";
        temp += RecodeGrammarFromYandex(encoding, lemma.GetFlexGram()[k], englishGrammar);
        TString t = SubstGramm(addGr.c_str(), lemma.GetFlexGram()[k]);
        if (!!t) {
            if (!temp.empty())
                temp += ",";
            temp += RecodeGrammarFromYandex(encoding, t.c_str(), englishGrammar);
        }
    }
    out += temp;
    out += " ";

    out += RecodeFromTChar(encoding, lemma.GetNormalizedForm(), lemma.GetNormalizedFormLength());
    out += " ";
    out += ToString(lemma.GetTokenPos());
    out += " ";
    out += ToString(lemma.GetTokenSpan());
    out += " ";
    out += IsoNameByLanguage(lemma.GetLanguage());
    out += " ";

    temp = "";
    for (int k = 0; k < 32; k++) {
        TCharCategory flag = (1<<k);
        if (lemma.GetCaseFlags() & flag) {
            if (case_flag_names.find(flag) != case_flag_names.end()) {
                if (!temp.empty()) temp += ",";
                temp += case_flag_names[flag];
            }
        }
    }

    out += temp;

    if (printWeight) {
        out += " ";
        out += ToString(lemma.GetWeight());
        out += " ";
        out += ToString(result.second);
    }

    if (printParadigm) {
        out += " ";
        out += PrintParadigm(encoding, lemma, ",", englishGrammar, printWeight);
    }

    return out;
}

void PrintComputerReadable(IOutputStream& out, const TLemmerResults& results, const TPrintParams& params)
{
    for (size_t j = 0; j < results.size(); j++)
        out << ComputerReadable(params.Encoding,
                results[j],
                params.CaseFlagNames,
                params.PrintParadigm,
                true,
                params.EnglishGrammar,
                params.DecomposeLemma,
                params.PrintWeight)
             << "\n";
    out << Endl;
}

void PrintTesting(IOutputStream& out, const TLemmerResults& results, const TPrintParams& params)
{
    TVector<TString> v;
    for (size_t j = 0; j < results.size(); j++)
        v.push_back(ComputerReadable(params.Encoding, results[j], params.CaseFlagNames, false, false, params.EnglishGrammar, false, params.PrintWeight));
    std::stable_sort(v.begin(), v.end());
    for (size_t j = 0; j < v.size(); j++)
        out << v[j] << "\n";

    out << Endl;
}

void PrintHumanReadable(IOutputStream& out, const TLemmerResults& results, const TPrintParams& params)
{
    for (size_t j = 0; j < results.size(); j++) {
        const TYandexLemma& lemma = results[j].first;
        if (params.Verbosity > 2) {
            out << "Form: ";
            size_t stemOff = lemma.GetPrefLen();
            size_t stemLen = lemma.GetNormalizedFormLength() - lemma.GetPrefLen() - lemma.GetFlexLen();
            if (lemma.GetPrefLen() > 0) {
                out << RecodeFromTChar(TUtf16String(lemma.GetNormalizedForm(), lemma.GetNormalizedForm() + stemOff), params.Encoding);
                out << "|";
            }
            out << RecodeFromTChar(TUtf16String(lemma.GetNormalizedForm() + stemOff, lemma.GetNormalizedForm() + stemOff + stemLen), params.Encoding);
            if (lemma.GetFlexLen() > 0) {
                out << "|";
                out << RecodeFromTChar(TUtf16String(lemma.GetNormalizedForm() + stemOff + stemLen, lemma.GetNormalizedForm() + lemma.GetNormalizedFormLength()), params.Encoding);
            }
            out << "\n";
        }
        if (params.Verbosity > 2) {
            size_t first = lemma.GetTokenPos();
            size_t last = lemma.GetTokenPos() + lemma.GetTokenSpan() - 1;
            out << "Span (in tokens): [" << ((int)first + 1) << ":" << ((int)last + 1) << "]" << "\n";
        }

        if (lemma.GetQuality() & TYandexLemma::QPrefixoid) {
            if (lemma.GetQuality() & TYandexLemma::QSob)
                out << "Sob prefixoid lemma";
            else
                out << "Prefixoid lemma";
        } else if ((lemma.GetQuality() & TYandexLemma::QBastard) && (lemma.GetQuality() &  TYandexLemma::QOverrode)) {
            out << "Quasibastard lemma";
        } else if (lemma.GetQuality() & TYandexLemma::QBastard) {
            out << "Bastard lemma";
        } else if (lemma.GetQuality() & TYandexLemma::QFoundling) {
            out << "Foundling lemma";
        } else if (lemma.GetQuality() & TYandexLemma::QSob) {
            out << "Sob lemma";
        } else if (lemma.GetQuality() & TYandexLemma::QSob) {
            out << "Fix list lemma";
        } else {
            out << "Lemma";
        }
        {
            bool flBr = false;
            if (lemma.GetQuality() & TYandexLemma::QBadRequest) {
                    if (!flBr)
                        out << " (";
                    else
                        out << ", ";
                    flBr = true;
                    out << "BadRequest";
            }
            if (lemma.LooksLikeLemma()) {
                    if (!flBr)
                        out << " (";
                    else
                        out << ", ";
                    flBr = true;
                    out << "LooksLikeLemma";
            }
            if (flBr)
                out << ")";
        }
        out << ": ";

        if (params.DecomposeLemma)
            out << RecodeFromTChar(NormalizeUnicode(TUtf16String(lemma.GetText(), lemma.GetTextLength())), params.Encoding);
        else
            out << RecodeFromTChar(params.Encoding, lemma.GetText(), lemma.GetTextLength());
        out << "\n";

        if (params.Verbosity > 2) {
            out << "Rule ID: " << NLemmerAux::GetRuleId(lemma) << "\n";
            out << "Lemma weight: " << lemma.GetWeight() << "\n";
        }
        if (params.Verbosity > 1) {
            if (lemma.GetStemGram())
                out << "Lemma features: " << RecodeGrammarFromYandex(params.Encoding, lemma.GetStemGram(), params.EnglishGrammar) << "\n";
            if (lemma.FlexGramNum()) {
                out << "Inflection features:";
                for (size_t k = 0; k < lemma.FlexGramNum(); k++) {
                    out << " " << RecodeGrammarFromYandex(params.Encoding, lemma.GetFlexGram()[k], params.EnglishGrammar);
                    //out << " " << lemma.GetDistortions()[k];
                }
                out << "\n";
            }
            if (lemma.GetAddGram()[0])
                out << "Additional features: " << RecodeGrammarFromYandex(params.Encoding, lemma.GetAddGram(), params.EnglishGrammar) << "\n";
        }
        out << "Language: " << FullNameByLanguage(lemma.GetLanguage()) << "\n";
        if (params.Verbosity > 2) {
            out << "Flags:";
            for (int k = 0; k < 32; k++) {
                TCharCategory flag = (1<<k);
                if (lemma.GetCaseFlags() & flag) {
                    if (params.CaseFlagNames.find(flag) != params.CaseFlagNames.end())
                        out << " " << params.CaseFlagNames.find(flag)->second;
                    else
                        out << " UNKNOWN[" << /*TODO hex <<*/ flag << "]";
                }
            }
            out << "\n";
        }
        if (params.PrintWeight) {
            out << "Weight: ";
            out << results[j].first.GetWeight();
            out << "\n";
        }
        if (params.PrintContextWeight) {
            out << "Context weight: ";
            out << results[j].second;
            out << "\n";
        }
        if (params.PrintParadigm) {
            out << "Paradigm:\n  ";
            out << PrintParadigm(params.Encoding, lemma, "\n  ", params.EnglishGrammar, params.PrintWeight);
            out << "\n";
        }
        out << Endl;
    }
}

struct TListerForm {
    TUtf16String Text;
    TUtf16String Prefix;
    TGramBitSet FlexGram;

    bool operator < (const TListerForm& other) const {
        return Prefix < other.Prefix ||
               Prefix == other.Prefix && FlexGram < other.FlexGram;
    }
};

void PrintLister(IOutputStream& out, const TLemmerResults& results) {
    TWordformArray forms;
    for (const auto& result : results) {
        const auto& lemma = result.first;
        out << "# " << FullNameByLanguage(lemma.GetLanguage()) << Endl;
        out << lemma.GetText() << Endl;

        forms.clear();
        NLemmer::Generate(lemma, forms);
        TVector<TListerForm> listerForms;
        for (const auto& form : forms) {
            const auto& formText = form.GetText();
            const auto& prefix = formText.substr(0, form.GetPrefixLen());
            const auto& stem = formText.substr(form.GetPrefixLen(), form.GetStemLen());
            const auto& flex = formText.substr(form.GetPrefixLen() + form.GetStemLen());
            const auto& stemGram = TGramBitSet::FromBytes(form.GetStemGram());
            const auto& stemGramStr = stemGram.ToString(",");
            for (size_t i = 0; i != form.FlexGramNum(); ++i) {
                const auto& flexGram = TGramBitSet::FromBytes(form.GetFlexGram()[i]);
                const auto& flexGramStr = flexGram.ToString(",");
                TString gramStr = stemGramStr + (stemGramStr.empty() || flexGramStr.empty() ? "" : ",") + flexGramStr;
                TUtf16String text = prefix + u"[" + stem + u"]" + flex + u"\t" + ASCIIToWide(gramStr);
                listerForms.push_back(TListerForm{text, prefix, flexGram});
            }
        }
        Sort(listerForms.begin(), listerForms.end());
        for (const auto& listerForm : listerForms)
            out << listerForm.Text << Endl;
        out << Endl;
    }
}



int SelectOutput(IInputStream& in, const TParams& params) {
    if (params.OutFile.empty()) {
        ProcessFile(in, Cout, params);
        return 0;
    }

    THolder<IOutputStream> out;

    try {
        out.Reset(new TFixedBufferFileOutput(params.OutFile.c_str()));
    } catch (...) {
        Cerr << CurrentExceptionMessage() << ": " << params.OutFile << Endl;

        return 1;
    }

    ProcessFile(in, *out, params);

    return 0;
}

int SelectInput(const TParams& params) {
    if (params.InFile.empty())
        return SelectOutput(Cin, params);

    THolder<IInputStream> in;
    try {
        in.Reset(new TFileInput(params.InFile.c_str()));
    } catch (...) {
        Cerr << CurrentExceptionMessage() << ": " << params.InFile << Endl;
        return 1;
    }
    int result = SelectOutput(*in, params);
    return result;
}

void PrintUsageAndExit(const NLastGetopt::TOptsParser* parser) {
    parser->PrintUsage(Cerr);

    Cerr << "If no input file is specified, data is read from stdin." << Endl;
    Cerr << "If no output file is specified, results are sent to stdout." << Endl;

    exit(0);
}

void PrintSupportedLanguages() {
    Cout << "The following languages are currently supported:" << Endl;

    const NLemmer::TLanguageVector& langlist = NLemmer::GetLanguageList();
    NLemmer::TLanguageVector::const_iterator it = langlist.begin();
    while (it != langlist.end()) {
        const TLanguage *lang = *(it++);
        Cout << "    " << lang->Code() << ": " << lang->Name() << (lang->GetVersion() > 0 ? " (new)" : "" ) << Endl;
    }
}

int main(int argc, char* argv[])
{
    TParams params;
    bool printFingers = false;
    bool printSupportedLanguages = false;
    ELanguage robotLanguage = LANG_MAX;

    NLastGetopt::TOpts opts;
    opts.AddLongOption('h', "help", "Print this help message and exit")
        .Handler(&PrintUsageAndExit)
        .NoArgument();
    opts.AddCharOption('e', "Character encoding;")
        .DefaultValue("utf-8")
        .RequiredArgument("<encoding>");
    opts.AddCharOption('l', "Languages preference order (comma-separated)")
        .RequiredArgument("<languages>");
    opts.AddCharOption('m', "List of accepted languages (comma-separated)")
        .RequiredArgument("<languages>");
    opts.AddCharOption('o', "Output file")
        .StoreResult(&params.OutFile)
        .RequiredArgument("<file>");
    opts.AddCharOption('v', "Verbosity level: from -1 (nothing) to 3 (everything);")
        .StoreResult(&params.PrintParams.Verbosity)
        .DefaultValue("2")
        .RequiredArgument("<level>");
    opts.AddCharOption('g', "List of accepted grammemes (comma-separated)")
        .RequiredArgument("<grammar>");
    opts.AddCharOption('p', "Print the whole paradigm")
        .StoreResult(&params.PrintParams.PrintParadigm, true)
        .NoArgument();
    opts.AddCharOption('c', "Computer readable output; disables -v")
        .StoreResult(&params.CompReadable, true)
        .NoArgument();
    opts.AddCharOption('t', "Testing output; disables -v, -c && -p")
        .StoreResult(&params.TestingOutput, true)
        .NoArgument();
    opts.AddCharOption('s', "Stopwords file")
        .RequiredArgument("<stopwords>");
    opts.AddCharOption('f', "Fixlist file")
        .RequiredArgument("<fixlist>");
    opts.AddCharOption('w', "Print analyses from WordInstance instead of AnalyzeWord")
        .StoreResult(&params.WordInstance, true)
        .NoArgument();
    opts.AddCharOption('d', "Print delimiter after each input line")
        .StoreResult(&params.PrintLine, true)
        .NoArgument();
    opts.AddCharOption('b', "Turn on bastards in all languages")
        .StoreResult(&params.AllBastards, true)
        .NoArgument();
    opts.AddCharOption('q', "Generate quasibastards")
        .StoreResult(&params.GenerateQuasiBastards, true)
        .NoArgument();
    opts.AddCharOption('a', "Generate all bastards")
        .StoreResult(&params.GenerateAllBastards, true)
        .NoArgument();
    opts.AddCharOption('r', "Number of the last productive rule (for all dictionaries)")
        .StoreResult(&params.LastNonRareRuleId)
        .RequiredArgument("<rule id>");
    opts.AddLongOption("tryutf", "Try to convert input from utf8 first")
        .StoreResult(&params.TryUtfInput, true)
        .NoArgument();
    opts.AddLongOption("russian", "Print grammemes in Russian")
        .StoreResult(&params.PrintParams.EnglishGrammar, false)
        .NoArgument();
    opts.AddLongOption("fingerprints", "Print languages fingerprints and exit")
        .StoreResult(&printFingers, true)
        .NoArgument();
    opts.AddLongOption("decompose", "Normalize lemma (as in index)")
        .StoreResult(&params.PrintParams.DecomposeLemma, true)
        .NoArgument();
    opts.AddLongOption("indexer", "Use indexer options for AnalyzeWord")
        .RequiredArgument("<language>");
    opts.AddLongOption("trans", "Use translit and translate")
        .StoreResult(&params.Trans, true)
        .NoArgument();
    opts.AddLongOption("disamb")
        .Help(
            "Available options:\n"
            "    zel - ZelDisamb, is default one.\n"
            "    mx  - MatrixNet"
        )
        .StoreResult(&params.Disambiguate)
        .OptionalArgument("<disamb>");
    opts.AddLongOption("spellcheck", "Call Spellcheck only")
        .StoreResult(&params.Spellcheck, true)
        .NoArgument();
    opts.AddLongOption("old_tokenization", "Use old marks tokenization")
        .StoreResult(&params.UseOldMarkTokenization, true)
        .NoArgument();
    opts.AddLongOption("decimator", "Decimator config for -pw mode")
        .StoreResult(&params.DecimatorConfig)
        .OptionalArgument("<config>");
    opts.AddLongOption("weights", "Print form weights in paradigm")
        .StoreResult(&params.PrintParams.PrintWeight, true)
        .NoArgument();
    opts.AddLongOption("skip-tokenization", "Treat whole line as single word")
        .StoreResult(&params.SkipTokenization, true)
        .NoArgument();
    opts.AddLongOption("skip-normalization", "Skip letters normalization step")
        .StoreResult(&params.SkipNormalization, true)
        .NoArgument();
    opts.AddLongOption("do-not-prune", "Do not prune lemmas with low probability")
        .StoreResult(&params.DoNotPrune, true)
        .NoArgument();
    opts.AddLongOption("lister-output", "Print paradigm in lister format")
        .StoreResult(&params.ListerOutput, true)
        .NoArgument();
    opts.AddLongOption("supported-languages", "Print list of supported languages and exit")
        .StoreResult(&printSupportedLanguages, true)
        .NoArgument();
    opts.AddLongOption("automorphology", "Load automorphology, can be specified multiple times.")
        .RequiredArgument("<lang=path>")
        .KVHandler([](TString lang, TString path) {
            TAutomorphologyLanguage::LoadLanguage(LanguageByNameStrict(lang), path);
        });

    opts.SetFreeArgsMin(0);
    opts.SetFreeArgsMax(1);
    opts.SetFreeArgTitle(0, "<input file>", "input file");


    NLastGetopt::TOptsParseResult res(&opts, argc, argv);

    if (res.Has('e')) {
        params.PrintParams.Encoding = CharsetByName(res.Get('e'));
        if (params.PrintParams.Encoding == CODES_UNKNOWN) {
            Cerr << "Unrecognized encoding: \"" << res.Get('e') << "\"" << Endl;
            return 1;
        }
    }
    if (res.Has('l')) {
        params.SetLangList(res.Get('l'));
    }
    if (res.Has('m')) {
        params.SetLangMask(res.Get('m'));
    }
    if (res.Has('g')) {
        params.SetAcceptedGrammemes(res.Get('g'));
    }
    if (res.Has('s')) {
        params.SetStopWordList(res.Get('s'));
    }
    if (res.Has('f')) {
        NLemmer::SetMorphFixList(res.Get('f'));
    }
    if (res.Has("indexer")) {
        params.RobotOptions = true;
        robotLanguage = LanguageByNameStrict(res.Get("indexer"));
        if (robotLanguage == LANG_MAX) {
            Cerr << "Unknown language: \"" << res.Get("indexer") << "\"" << Endl;
            return 1;
        }
    }
    if (res.Has("disamb")) {
        params.PrintParams.PrintWeight = true;
        params.PrintParams.PrintContextWeight = true;
        if (params.Disambiguate.empty()) {
            params.Disambiguate = "zel";
        }
        if (params.Disambiguate != "zel" && params.Disambiguate != "mx") {
            Cerr << "Unknown disamb type: "<< params.Disambiguate << ". Available are: zel, mx" << Endl;
            return 1;
        }
    }

    if (robotLanguage < LANG_MAX)
        params.SetRobotLanguage(robotLanguage);

    if (printFingers) {
        printFingerprints(params.LanguagesMask);
        return 0;
    }

    if (printSupportedLanguages) {
        PrintSupportedLanguages();
        return 0;
    }

    TVector<TString> freeArgs = res.GetFreeArgs();
    if (freeArgs.size() > 0)
        params.InFile = freeArgs[0];

    return SelectInput(params);
}

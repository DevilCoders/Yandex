#include <util/generic/hash.h>
#include <util/generic/vector.h>
#include <util/generic/singleton.h>
#include <util/generic/string.h>

#include <kernel/indexer/lexical_decomposition/token_lexical_splitter.h>
#include <kernel/translate/translate.h>

#include <kernel/lemmer/untranslit/untranslit.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/translate/translatedict.h>
#include <library/cpp/langmask/index_language_chooser.h>

#include "url_transliterator.h"

void TUrlTransliteratorItem::ConvertPartToLower() {
    // Part can't contain digits and letters simultaneously, see TURLTokenizer
    if (IsDigit(Part[0])) {
        Titled = false;
        return;
    }

    if (!IsUpper(Part[0])) {
        Titled = false;
        Part.to_lower();
        return;
    }

    const wchar16* p = Part.begin() + 1; // skip the first letter
    const wchar16* const e = Part.end();
    bool titled = true;

    while (p != e) {
        if (!IsLower(*p)) {
            titled = false;
            break;
        }
        ++p;
    }

    Titled = titled;
    Part.to_lower();
}

const TUtf16String& TUrlTransliteratorItem::GetLowerCasePart() const {
    return Part;
}

bool TUrlTransliteratorItem::IsPartTitled() const {
    return Titled;
}

void TUrlTransliteratorItem::SetPart(const TUtf16String& part) {
    Part = part;
    ConvertPartToLower();
}

void TUrlTransliteratorItem::SetForma(const TUtf16String& form) {
    Form = form;
}
const TUtf16String& TUrlTransliteratorItem::GetForma() const {
    return Form;
}

const TUtf16String& TUrlTransliteratorItem::GetLowerCaseLemma() const {
    return Lemma;
}

void TUrlTransliteratorItem::SetLemma(const TUtf16String& lemma) {
    Lemma = lemma;
    Lemma.to_lower();
}

ELanguage TUrlTransliteratorItem::GetLanguage() const {
    return Language;
}

void TUrlTransliteratorItem::SetLanguage(ELanguage lang) {
    Language = lang;
}

size_t TUrlTransliteratorItem::GetIndex() const {
    return Index;
}

void TUrlTransliteratorItem::SetIndex(size_t index) {
    Index = index;
}

bool TUrlTransliteratorItem::GetInCGI() const {
    return InCGI;
}

void TUrlTransliteratorItem::SetInCGI(bool value) {
    InCGI = value;
}

TUrlTransliteratorItem::ELemmaType TUrlTransliteratorItem::GetLemmaType() const {
    return LemmaType;
}

void TUrlTransliteratorItem::SetLemmaType(TUrlTransliteratorItem::ELemmaType lemmaType) {
    LemmaType = lemmaType;
}


/////////////////////////////////////////////////

void TSerializer<TUrlTransliteratorItem>::Load(IInputStream* in, TUrlTransliteratorItem& item) {
    ::Load(in, item.Part);
    ::Load(in, item.Form);
    ::Load(in, item.Lemma);
    ::Load(in, item.Language);
    ::Load(in, item.Index);
    ::Load(in, item.InCGI);
    ::Load(in, item.Titled);
    ::Load(in, item.LemmaType);
}

void TSerializer<TUrlTransliteratorItem>::Save(IOutputStream* out, const TUrlTransliteratorItem& item) {
    ::Save(out, item.Part);
    ::Save(out, item.Form);
    ::Save(out, item.Lemma);
    ::Save(out, item.Language);
    ::Save(out, item.Index);
    ::Save(out, item.InCGI);
    ::Save(out, item.Titled);
    ::Save(out, item.LemmaType);
}


/////////////////////////////////////////////////

static const TUrlTransliterator::TLemmas* EmptyLemmas() {
    return Singleton<TUrlTransliterator::TLemmas>();
};

TUrlTransliterator::TUrlTransliterator(TTransliteratorCache& transliteratorCache, const TString& url, ELanguage docLang, const TBlob* ptrTokSplitData)
    : TransliteratorCache(transliteratorCache)
    , Tokenizer(url.c_str(), url.size(), false)
    , URLPartIndex(0)
    , MixedCaseTokenCount(0)
    , PriorityLang(docLang)
    , TokSplitData(ptrTokSplitData)
    , Number(1, TLemma()) // size is not changed
    , Lemmas(EmptyLemmas())
    , LemmaIndex(0)
{
    HasNextToken = Advance();
}

bool TUrlTransliterator::Has() {
    return HasNextToken;
}

void TUrlTransliterator::Next(TUrlTransliteratorItem& item) {
    Y_ASSERT(Has());
    const size_t index = URLPartIndex - 1;
    const TURLTokenizer::TToken& token = Tokenizer.GetToken(index);
    item.SetPart(TUtf16String(token.data(), token.size()));
    item.SetForma(NextToken.Forma);
    item.SetLemma(NextToken.Lemma);
    item.SetLanguage(NextToken.Language);
    item.SetIndex(index);
    item.SetInCGI(Tokenizer.IsPartOfQuery(index));
    item.SetLemmaType(NextToken.Type);
    HasNextToken = Advance();
}

// Well, this is not a good place for dictionaries
namespace {
    class CUntrDict: public THashSet<TUtf16String> {
    public:
        bool Has(const TUtf16String& s) const {
            return find(s) != end();
        }
    };
#define I(s) insert(ASCIIToWide(s))
    class CStopCGIParts: public CUntrDict {
    public:
        CStopCGIParts() {
            I("action");
            I("article");
            I("ba");
            I("bb");
            I("bc");
            I("bd");
            I("be");
            I("bf");
            I("cat");
            I("category");
            I("catid");
            I("ce");
            I("cid");
            I("com");
            I("content");
            I("deu");
            I("ea");
            I("eb");
            I("ec");
            I("ed");
            I("ee");
            I("ef");
            I("id");
            I("itemid");
            I("lang");
            I("mode");
            I("name");
            I("news");
            I("option");
            I("order");
            I("page");
            I("pagen");
            I("pg");
            I("pid");
            I("print");
            I("ru");
            I("search");
            I("showtopic");
            I("sid");
            I("sort");
            I("st");
            I("start");
            I("task");
            I("threaded");
            I("topic");
            I("type");
            I("ua");
            I("uid");
            I("uk");
            I("view");
            I("year");
        }
    };
#undef I
    bool StopCGIParts(const TUtf16String& w) {
        return Singleton<CStopCGIParts>()->Has(w);
    }
}

namespace {
    typedef std::pair<TUtf16String, std::pair<TLangMask, TLangMask> > Twl;
    struct THashWtrLang {
        size_t operator()(const Twl& key) const {
            return ComputeHash(key.first) + key.second.first.GetHash() + key.second.second.GetHash();
        }
    };
}

static TLangMask AdditionalTranslitLangMask(ELanguage lang) {
    if (CyrillicScript(lang) && !TIndexLanguageOptions::CyrillicLanguagesPrecludesRussian(lang) || lang == LANG_ENG || lang == LANG_UNK)
        return TLangMask(LANG_RUS);
    return TLangMask();
}

static TLangMask AdditionalTranslateLangMask(ELanguage lang) {
    if (lang == LANG_BEL || lang == LANG_KAZ || lang == LANG_UNK)
        return TLangMask(LANG_RUS);
    return TLangMask();
}

struct TUrlTransliterator::TTransliteratorCache::TUrlRequest { //just not to type it again and again
    bool InCGI;
    ELanguage PriorityLangId;
    const TLanguage* PriorityLang;
    TLangMask AdditionalTranslitLangMask;
    TLangMask AdditionalTranslateLangMask;
    TUtf16String InitWord;
    TUtf16String MainLowWord;

    TUrlRequest(bool inCGI, ELanguage priorLang, const TURLTokenizer::TToken& token);

    Twl GetKey() const {
        TUtf16String key = InitWord + (wchar16)(InCGI ? '1' : '0');
        key += wchar16(' ' + PriorityLangId);
        return Twl(key, std::pair<TLangMask, TLangMask>(AdditionalTranslitLangMask, AdditionalTranslateLangMask));
    }
};

class TUrlTransliterator::TTransliteratorCache::TImpl {
public:
    const static size_t MAX_SIZE = 256*1024*1024;

    TImpl()
        : URLPart2LemmasSize(0)
    {
    }

    const TUrlTransliterator::TLemmas* Transliterate(const TUrlRequest& req, const TBlob* tokSplitData);
private:
    typedef THashMap<Twl, TUrlTransliterator::TLemmas, THashWtrLang> TWtroka2Lemmas;
    TWtroka2Lemmas URLPart2Lemmas;
    size_t URLPart2LemmasSize;
};

TUrlTransliterator::TTransliteratorCache::TTransliteratorCache()
    :Impl(new TImpl)
{
}

TUrlTransliterator::TTransliteratorCache::~TTransliteratorCache() {
}

const TUrlTransliterator::TLemmas* TUrlTransliterator::TTransliteratorCache::Transliterate(const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req, const TBlob* tokSplitData) {
    return Impl->Transliterate(req, tokSplitData);
}

namespace {
    TUtf16String LowCaseLang(const TUtf16String& in, ELanguage langId) {
        TUtf16String out = in;
        NLemmer::GetAlphaRules(langId)->ToLower(out);
        return out;
    }

    TUrlTransliterator::TLemma ConstructLemma(const TUtf16String& lemma, const TUtf16String& form, ELanguage lang, TUrlTransliteratorItem::ELemmaType type) {
        TUrlTransliterator::TLemma lm = {lemma, form, lang, type};
        return lm;
    }

    TUrlTransliterator::TLemma ConstructLemma(const TYandexLemma& lemma, TUrlTransliteratorItem::ELemmaType type) {
        TUrlTransliterator::TLemma lm = {
            TUtf16String(lemma.GetText(), lemma.GetTextLength()),
            TUtf16String(lemma.GetNormalizedForm(), lemma.GetNormalizedFormLength()),
            lemma.GetLanguage(), type};
        return lm;
    }

    bool checkLemma(const TUrlTransliterator::TLemma& /*lemma*/) {
        return true;
    }
    size_t AddLemma(TUrlTransliterator::TLemmas& tLemmas, const TUrlTransliterator::TLemma& lemma) {
        if (!checkLemma(lemma))
            return 0;
        tLemmas.push_back(lemma);
        tLemmas.back().Lemma.to_lower(); // should take no effect
        return 1;
    }
}

TUrlTransliterator::TTransliteratorCache::TUrlRequest::TUrlRequest(bool inCGI, ELanguage priorLang, const TURLTokenizer::TToken& token)
    : InCGI(inCGI)
    , PriorityLangId(priorLang)
    , PriorityLang(NLemmer::GetLanguageById(priorLang))
    , AdditionalTranslitLangMask(::AdditionalTranslitLangMask(PriorityLangId))
    , AdditionalTranslateLangMask(::AdditionalTranslateLangMask(PriorityLangId))
    , InitWord(token.data(), token.size())
    , MainLowWord(LowCaseLang(InitWord, PriorityLangId))
 {}

static size_t TranslateUrlLang(TUrlTransliterator::TLemmas& tLemmas, const TUtf16String& word, const TLanguage* lang, size_t max) {
    TWLemmaArray lemmas;
    if (!lang)
        return 0;
    //lang->FromEnglish(~word, +word, lemmas, max);
    NTranslate::FromEnglish(word, lang->Id, lemmas, max);
    size_t cResults = 0;
    for (size_t i = 0; i < lemmas.size(); ++i)
        cResults += AddLemma(tLemmas, ConstructLemma(lemmas[i], TUrlTransliteratorItem::LtTranslate));
    return cResults;
}

static size_t TranslateUrlLemmas(TUrlTransliterator::TLemmas& tLemmas, const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req) {
    size_t cResults = TranslateUrlLang(tLemmas, req.MainLowWord, req.PriorityLang, (!req.InCGI ? 2 : 1));

    for (ELanguage lg : req.AdditionalTranslateLangMask) {
        Y_ASSERT(lg != req.PriorityLangId);
        const TLanguage* lang = NLemmer::GetLanguageById(lg);
        cResults += TranslateUrlLang(tLemmas, LowCaseLang(req.InitWord, lg), lang, 1);
    }
    return cResults;
}

static size_t DirectTranslitUrlLemmas(TUrlTransliterator::TLemmas& tLemmas, const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req) {
    if (!req.PriorityLang)
        return 0;
    TAutoPtr<TUntransliter> un = req.PriorityLang->GetUntransliter(req.MainLowWord);
    if (!un)
        return 0;
    TUntransliter::WordPart wp;
    size_t cResults = 0;
    while ((cResults < 3) && !(wp = un->GetNextAnswer()).Empty()) {
        cResults += AddLemma(tLemmas, ConstructLemma(wp.GetWord(), req.MainLowWord, req.PriorityLang->Id, TUrlTransliteratorItem::LtTranslit));
    }
    return cResults;
}

static size_t DictUrlLemmas(TUrlTransliterator::TLemmas& tLemmas, const TUtf16String& word, const TLanguage* priorLang, size_t max) {
    Y_ASSERT(priorLang);
    const TTranslationDict* dict = priorLang->GetUrlTransliteratorDict();
    if (!dict)
        return 0;

    TVector<TTranslationDict::TArticle> trRes;
    if (!dict->FromEnglish(word.c_str(), trRes))
        return 0;

    TWLemmaArray lemmas;
    size_t cResults = 0;
    for (size_t i1 = 0; i1 < trRes.size() && cResults < max; ++i1) {
        if (trRes[i1].Word[0] == '!') {
            TUtf16String wrd(trRes[i1].Word + 1);
            cResults += AddLemma(tLemmas, ConstructLemma(wrd, wrd, priorLang->Id, TUrlTransliteratorItem::LtTranslit));
            continue;
        }
        NLemmer::AnalyzeWord(trRes[i1].Word, std::char_traits<wchar16>::length(trRes[i1].Word), lemmas, TLangMask(priorLang->Id), nullptr, NLemmer::TAnalyzeWordOpt::IndexerOpt());
        for (size_t i2 = 0; i2 < lemmas.size(); ++i2) {
            cResults += AddLemma(tLemmas, ConstructLemma(lemmas[i2], TUrlTransliteratorItem::LtTranslit));
            if (cResults >= max)
                return cResults;
        }
    }
    return cResults;
}

static std::pair<size_t, bool> TranslitLang(TUrlTransliterator::TLemmas& tLemmas, const TUtf16String& word, const TLanguage* priorLang, bool accepBast, size_t max) {
    if (!priorLang)
        return std::pair<size_t, bool>();
    size_t n = DictUrlLemmas(tLemmas, word, priorLang, max);
    if (n)
        return std::pair<size_t, bool>(n, true);

    std::pair<size_t, bool> ret(0, false);
    TWLemmaArray lemmas;
    NLemmer::TTranslitOpt opt = NLemmer::TAnalyzeWordOpt::IndexerOpt().GetTranslitOptions(priorLang->Id);
    priorLang->FromTranslit(word.data(), word.size(), lemmas, max, &opt);
    if (!lemmas.empty() && !lemmas[0].IsBastard())
        ret.second = true;
    if (lemmas.empty() || !accepBast && lemmas[0].IsBastard())
        return ret;
    for (size_t i = 0; i < lemmas.size(); ++i)
        ret.first += AddLemma(tLemmas, ConstructLemma(lemmas[i], TUrlTransliteratorItem::LtTranslit));
    return ret;
}

static size_t TranslitUrlLemmas(TUrlTransliterator::TLemmas& tLemmas, const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req) {
    std::pair<size_t, bool> trRet = TranslitLang(tLemmas, req.MainLowWord, req.PriorityLang, true, (!req.InCGI ? 4 : 1));
    size_t cResults = trRet.first;
    bool flPrim = !cResults;
    if (flPrim)
        cResults += DirectTranslitUrlLemmas(tLemmas, req);
    for (ELanguage lg : req.AdditionalTranslitLangMask) {
        Y_ASSERT(lg != req.PriorityLangId);
        const TLanguage* lang = NLemmer::GetLanguageById(lg);
        cResults += TranslitLang(tLemmas, LowCaseLang(req.InitWord, lg), lang, flPrim, 1).first;
    }
    return cResults;
}

static size_t SplitUrlWords(TUrlTransliterator::TLemmas& tLemmas, const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req, const TBlob* tokSplitData) {
    size_t cResults = 0;
    ELanguage curLang(LANG_UNK);
    TLangMask langmask = NLemmer::ClassifyLanguage(req.InitWord.c_str(), req.InitWord.length());
    langmask &= (TLangMask(LANG_ENG, req.PriorityLangId) | req.AdditionalTranslitLangMask);
    TUrlTransliteratorItem::ELemmaType curType(TUrlTransliteratorItem::LtOriginal);
    NLexicalDecomposition::TTokenLexicalSplitter tokSplitter(*tokSplitData);
    for (ELanguage id : langmask) {
        if (tokSplitter.ProcessToken(LowCaseLang(req.InitWord, id), id)) {
            curLang = id;
            curType = TUrlTransliteratorItem::LtOriginal;
        }
    }
    for (size_t i = 0; i < tLemmas.size(); ++i) {
        if (tokSplitter.ProcessToken(tLemmas[i].Lemma, tLemmas[i].Language)) {
            curLang = tLemmas[i].Language;
            curType = TUrlTransliteratorItem::LtTranslit;
        }
    }

    if (tokSplitter.GetResultSize() <= 1) return 0;

    for (size_t j = 0; j < tokSplitter.GetResultSize(); ++j) {
        TUtf16String word = tokSplitter[j];
        cResults += AddLemma(tLemmas, ConstructLemma(word, word, curLang, curType));
    }

    return cResults;
}

static size_t MakeUrlLemmas(TUrlTransliterator::TLemmas& tLemmas, const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req, const TBlob* tokSplitData) {
    size_t cResults = 0;
    if (!req.InCGI || req.InitWord.size() > 1) {
        cResults += AddLemma(tLemmas, ConstructLemma(req.MainLowWord, req.MainLowWord, LANG_UNK, TUrlTransliteratorItem::LtOriginal));
    }

    const static size_t MAX_URL_PART_LENGTH = 32;
    if (req.InitWord.size() > MAX_URL_PART_LENGTH)
        return cResults;

    if (req.InCGI && (StopCGIParts(req.MainLowWord) || StopCGIParts(LowCaseLang(req.InitWord, LANG_UNK))))
        return cResults;

    if (req.InCGI && (req.InitWord.size() <= 2))
        return cResults;

    cResults += TranslitUrlLemmas(tLemmas, req);
    if (tokSplitData && !tokSplitData->Empty())
        cResults += SplitUrlWords(tLemmas, req, tokSplitData);
    cResults += TranslateUrlLemmas(tLemmas, req);

    return cResults;
}

const TUrlTransliterator::TLemmas* TUrlTransliterator::TTransliteratorCache::TImpl::Transliterate(const TUrlTransliterator::TTransliteratorCache::TUrlRequest& req, const TBlob* tokSplitData) {
    const Twl hashKey = req.GetKey();
    TWtroka2Lemmas::const_iterator toURLPart = URLPart2Lemmas.find(hashKey);
    if (toURLPart != URLPart2Lemmas.end())
        return &(toURLPart->second);

    TUrlTransliterator::TLemmas tLemmas;
    MakeUrlLemmas(tLemmas, req, tokSplitData);

    if (URLPart2LemmasSize > MAX_SIZE) {
        URLPart2Lemmas.clear();
        URLPart2LemmasSize = 0;
    }
    for (size_t i = 0; i < tLemmas.size(); ++i)
        URLPart2LemmasSize += tLemmas[i].Lemma.size() + 1;

    toURLPart = URLPart2Lemmas.insert(TWtroka2Lemmas::value_type(hashKey, tLemmas)).first;
    return &toURLPart->second;
}

bool TUrlTransliterator::Advance() {
    while (LemmaIndex == Lemmas->size() && URLPartIndex < Tokenizer.GetTokenCount()) {
        const TURLTokenizer::TToken tok = Tokenizer.GetToken(URLPartIndex);
        if (IsDigit(*tok.data())) {
            Number[0].Lemma = Number[0].Forma = tok;
            Lemmas = &Number;
        } else {
            TCharCategory categ = NLemmer::ClassifyCase(tok.data(), tok.size());

            bool isMixed = categ & CC_MIXEDCASE;
            if (isMixed) {
                ++MixedCaseTokenCount;
            }

            // to avoid transliteration of too much base64 encoded trash
            // assume that all mixedcased tokens after first MAX_FIRST_MIXEDCASE_TO_TRANSLIT
            // are useless
            if (!isMixed || MixedCaseTokenCount <= MAX_FIRST_MIXEDCASE_TO_TRANSLIT){
                TTransliteratorCache::TUrlRequest req(Tokenizer.IsPartOfQuery(URLPartIndex), PriorityLang, tok);
                Lemmas = TransliteratorCache.Transliterate(req, TokSplitData);
            } else {
                Lemmas = EmptyLemmas();
            }
        }
        ++URLPartIndex;
        LemmaIndex = 0;
    }
    if (LemmaIndex != Lemmas->size()) {
        NextToken = (*Lemmas)[LemmaIndex++];
        return true;
    } else
        return false;
}

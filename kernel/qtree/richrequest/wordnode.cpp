#include <kernel/search_types/search_types.h>
#include "wordnode.h"
#include "lemmer_cache.h"
#include "init.h"

#include <kernel/qtree/richrequest/protos/rich_tree.pb.h>

#include <library/cpp/charset/recyr.hh>
#include <library/cpp/langmask/serialization/langmask.h>

#include <util/charset/wide.h>
#include <util/generic/yexception.h>
#include <util/string/vector.h>
#include <utility>

#ifdef LOG_LEMMER_CALLS
#include <util/datetime/cputimer.h>
#endif

#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <kernel/lemmer/core/langcontext.h>
#include <library/cpp/token/charfilter.h>
#include <ysite/yandex/common/prepattr.h>

namespace {
    typedef const std::pair<const TUtf16String, TLemmaForms::TFormWeight>* TFormPtr;
    struct TFormsComparer {
        bool operator()(const TFormPtr& a, const TFormPtr& b) const {
            return a->first < b->first;
        }
    };
    struct TVSortedForms: public TVector<TFormPtr> {
        TVSortedForms(const TLemmaForms& lm) {
            reserve(lm.GetForms().size());
            for (TLemmaForms::TFormMap::const_iterator i = lm.GetForms().begin(); i != lm.GetForms().end(); ++i)
                push_back(&*i);
            Sort(begin(), end(), TFormsComparer());
        }
    };
}

bool SoftCompare(const TLemmaForms& a, const TLemmaForms& b) {
    if (a.GetLemma() != b.GetLemma() || a.GetLanguage() != b.GetLanguage())
        return false;

    if (a.GetForms().size() != b.GetForms().size())
        return false;

    TVSortedForms sortedForms1(a);
    TVSortedForms sortedForms2(b);

    TVSortedForms::const_iterator it1 = sortedForms1.begin();
    TVSortedForms::const_iterator it2 = sortedForms2.begin();

    while (it1 != sortedForms1.end() && it2 != sortedForms2.end()) {
        if ((*it1)->first != (*it2)->first)
            return false;
        ++it1;
        ++it2;
    }
    if (it1 != sortedForms1.end() || it2 != sortedForms2.end())
        return false;
    return true;
}

using NLemmerAux::NormalizeForm;
using NLemmerAux::CmpCutWords;
using NLemmerAux::CutWord;

bool Compare(const TLemmaForms& a, const TLemmaForms& b) {
    TVSortedForms sortedForms1(a);
    TVSortedForms sortedForms2(b);

    TVSortedForms::const_iterator it1 = sortedForms1.begin();
    TVSortedForms::const_iterator it2 = sortedForms2.begin();

    while (it1 != sortedForms1.end() && it2 != sortedForms2.end()) {
        if (!CmpCutWords((*it1)->first, (*it2)->first) ||
            !((*it1)->second == (*it2)->second))
        {
            return false;
        }
        const TUtf16String& s = (*it1)->first;
        ++it1;
        ++it2;
        while (it1 != sortedForms1.end()) {
            if (!CmpCutWords((*it1)->first, s))
                break;
            ++it1;
        }
        while (it2 != sortedForms2.end()) {
            if (!CmpCutWords((*it2)->first, s))
                break;
            ++it2;
        }
    }
    if (it1 != sortedForms1.end() || it2 != sortedForms2.end())
        return false;

    return  1
         && CmpCutWords(a.GetLemma(), b.GetLemma())
         &&  a.GetLanguage() == b.GetLanguage()
         &&  a.IsBest() == b.IsBest()
         &&  (a.IsStopWord() == b.IsStopWord()
            && a.GetStickiness() == b.GetStickiness()
            && a.GetStemGrammar() == b.GetStemGrammar()
            && a.GetFlexGrammars() == b.GetFlexGrammars()
            && a.IsBastard() == b.IsBastard()
            && a.GetQuality() == b.GetQuality())
    ;
}

TWordNode::TWordNode()
    : LemType(LEM_TYPE_NON_LEMMER)
    , RevFr(-1)
    , UpDivLo(0.0)
{}

TFormType TWordNode::GetFormType() const {
    TFormType ft = TWordInstance::GetFormType();
    if (ft == fWeirdExactWord)
        ft = fGeneral;
    return ft;
}

void TWordNode::SetFormType(TFormType formType) {
    if (!IsLemmerWord()) {
        Y_ASSERT(Lemmas.size() == 1);
        SetNonLemmerFormType(formType);
    } else {
        TWordInstance::SetFormType(formType);
    }
}

TAutoPtr<TWordNode> TWordNode::CreateEmptyNode() {
    return new TWordNode;
}

TAutoPtr<TWordNode> TWordNode::CreateLemmerNode(const TUtf16String& word, const TCharSpan& span, TFormType ftype, const TLanguageContext& lang, bool generateForms) {
    return CreateLemmerNodeInt(word, span, ftype, lang, generateForms, true);
}

TAutoPtr<TWordNode> TWordNode::CreateLemmerNodeWithoutFixList(const TUtf16String& word, const TCharSpan& span, TFormType ftype, const TLanguageContext& lang, bool generateForms) {
    return CreateLemmerNodeInt(word, span, ftype, lang, generateForms, false);
}


TAutoPtr<TWordNode> TWordNode::CreateLemmerNodeInt(const TUtf16String& word, const TCharSpan& span, TFormType ftype, const TLanguageContext& lang, bool generateForms, bool useFixList)
{
    Y_ASSERT(!!word);

    if (!word) {
        Cerr << "incorrect wordnode initialization" << Endl;
        ythrow yexception() << "incorrect wordnode initialization";
    }

    Y_ASSERT(span.Len);
    Y_ASSERT(span.EndPos() <= word.length());

    TAutoPtr<TWordNode> res(new TWordNode);
    res->LemType = LEM_TYPE_LEMMER_WORD;

    TLemmerCache::TKey key(word, span, ftype, lang);
    const TLemmerCache* cache = lang.LemmerCache;
    bool langDisabled = lang.GetDisabledLanguages().HasAny(lang.GetLangMask());
#ifndef LOG_LEMMER_CALLS
    if (cache && cache->HasCorrectExternals(lang) && cache->Find(key, *res) && !langDisabled) {
        Y_ASSERT(res->AreFormsGenerated());
        if (!generateForms) {
            // AreFormsGenerated means they are generated for every lemma,
            // if they are generated for some lemmas, it returns false,
            // but we still should discard forms.
            res->DiscardForms();
        }
    } else {
        InitInstance(word, span, lang, ftype, generateForms, useFixList, *res);
    }
#else
    if (cache && cache->HasCorrectExternals(lang) && !langDisabled) {
        TStringStream message;
        message << key.ToJson() << "\t";
        TTimer timer(message.c_str()); // To be destroyed after InitInstance.
        InitInstance(word, span, lang, ftype, generateForms, useFixList, *res);
    } else {
        InitInstance(word, span, lang, ftype, generateForms, useFixList, *res);
    }
#endif
    return res;
}

TAutoPtr<TWordNode> TWordNode::CreateLemmerNode(const TVector<const TYandexLemma*>& lms, TFormType ftype, const TLanguageContext& lang, bool generateForms)
{
    Y_ASSERT(!lms.empty());
    TAutoPtr<TWordNode> res(new TWordNode);
    res->LemType = LEM_TYPE_LEMMER_WORD;
    res->Init(lms, lang, ftype, generateForms);
    if (lang.FilterForms) {
        TWordInstanceUpdate(*res).ShrinkIntrusiveBastards();
        TWordInstanceUpdate(*res).RemoveBadRequest();
    }
    if (res->NumLemmas() == 1) {
        res->CleanBestFlag();
    }
    return res;
}

TAutoPtr<TWordNode> TWordNode::CreateNonLemmerNodeInt(const TUtf16String& lemma, const TUtf16String& forma, TFormType ftype, int lemType)
{
    Y_ASSERT(!!lemma && !!forma);
    TAutoPtr<TWordNode> res(new TWordNode);

    res->LemType = lemType;
    res->Form = forma;
    NLemmer::GetAlphaRulesUnsafe(LANG_UNK)->ToLower(res->Form);
    res->NormalizedForm = (lemType == LEM_TYPE_INTEGER) ? res->Form : NormalizeForm(res->Form, LANG_UNK);
    if (res->NormalizedForm.length() > MAXKEY_BUF)
        res->NormalizedForm.resize(MAXKEY_BUF);

    wchar16 attrbuf[MAXKEY_BUF];
    size_t len = Min(size_t(MAXWORD_BUF-2), lemma.size()); // for backward compatibility
    len = NLemmer::GetAlphaRulesUnsafe(LANG_UNK)->ToLower(lemma.data(), len, attrbuf, MAXKEY_BUF).Length;
    res->Lemmas.push_back(TLemmaForms(res->Form, TUtf16String(attrbuf, len), LANG_UNK, false, TYandexLemma::QFoundling, ftype));
    res->Lemmas.back().AddFormaMerge(NormalizeForm(res->Form, LANG_UNK, false), TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
    res->Lemmas.back().AddFormaMerge(res->NormalizedForm, TLemmaForms::TFormWeight(TLemmaForms::ExactMatch));
    res->CleanBestFlag();

    return res;
}

TAutoPtr<TWordNode> TWordNode::CreateNonLemmerNode(const TUtf16String& forma, TFormType ftype)
{
    return CreateNonLemmerNodeInt(forma, forma, ftype, LEM_TYPE_NON_LEMMER);
}

TAutoPtr<TWordNode> TWordNode::CreateAttributeIntegerNode(const TUtf16String& forma, TFormType ftype) {
    wchar16 attrbuf[MAXKEY_BUF];
    PrepareInteger(forma.data(), attrbuf);
    return CreateNonLemmerNodeInt(attrbuf, forma, ftype, LEM_TYPE_INTEGER);
}

TAutoPtr<TWordNode> TWordNode::CreatePlainIntegerNode(const TUtf16String& forma, TFormType ftype) {
    return CreateNonLemmerNodeInt(forma, forma, ftype, LEM_TYPE_INTEGER);
}

TAutoPtr<TWordNode> TWordNode::CreateTextIntegerNode(const TUtf16String& forma, TFormType ftype, bool planeInteger) {
    if (planeInteger)
        return CreatePlainIntegerNode(forma, ftype);
    else
        return CreateAttributeIntegerNode(forma, ftype);
}

bool TWordNode::Compare(const TWordNode& other) const {
    EStickySide stickiness1 = STICK_NONE;
    EStickySide stickiness2 = STICK_NONE;
    bool stopWord1 = IsStopWord(stickiness1);
    bool stopWord2 = other.IsStopWord(stickiness2);

    if (0
        || !CmpCutWords(Form, other.Form)
        || !CmpCutWords(NormalizedForm, other.NormalizedForm)
        ||  LangMask != other.LangMask
        ||  CaseFlags != other.CaseFlags
        ||  stopWord1 != stopWord2
        ||  (stickiness1 != stickiness2)
        ||  (TokenCount != other.TokenCount)
        )
    {
        return false;
    }

    return ((other.LemType == LemType)
        && (other.GetFormType() == GetFormType())
        && (other.RevFr == RevFr)
        );
}

static TString SerializeBinaryGrammar(const TGramBitSet& gramm) {
    TString res;
    res.reserve(gramm.count());
    gramm.ToBytes(res);
    return res;
}

static TGramBitSet DeserializeBinaryGrammar(const TString& gramm) {
    TGramBitSet res;

    for (size_t i = 0, mi = gramm.size(); i != mi ; ++i) {
        EGrammar code = NTGrammarProcessing::ch2tg(gramm.at(i));
        res.SafeSet(code);
    }

    return res;
}

static TString SerializeReadableGrammar(const TGramBitSet& stemgram, const TVector<TGramBitSet>& flexgrams) {
    TString tag(stemgram.ToString(","));
    if (!flexgrams.empty()) {
        tag += "(";
        TVector<TGramBitSet>::const_iterator it;
        for (it = flexgrams.begin(); it != flexgrams.end(); ++it) {
            if (it != flexgrams.begin())
                tag += "|";
            tag += it->ToString(",");
        }
        tag += ")";
    }

    return tag;
}

namespace NOldMode___ {
    typedef ui16 TMode;
    static const TMode ModeAll = 0;
    static const TMode ModeSkipBadRequest = 0x01;
    static const TMode ModeSkipIntrusiveBastards = 0x02;
    static const TMode ModeExactWord = 0x10;
    static const TMode ModeExactLemma = 0x20;
/*
    static const TMode ModeFilterSyntaxFull = 0x40;
    static const TMode ModeFilterSyntaxPrep = 0x80;
    static const TMode ModeFilterSyntaxNPs = 0x100;
    static const TMode ModeNearestForms = 0x200;
    static const TMode ModeGoodLanguage = 0x400;
*/

    static const TMode GeneralRequestMode = ModeSkipBadRequest | ModeSkipIntrusiveBastards;

    static TMode Type2Mode(TFormType type) {
        switch (type) {
            case fExactLemma:
                return (GeneralRequestMode | ModeExactLemma);
            case fExactWord:
                return (GeneralRequestMode | ModeExactWord);
            case fGeneral:
            case fWeirdExactWord:
                return GeneralRequestMode;
        }
        Y_ASSERT(!"Unknown form type");
        return GeneralRequestMode;
    }

    static TMode Type2DesMode(TFormType type) {
        switch (type) {
            case fExactLemma:
                return ModeExactLemma | ModeExactWord;
            case fExactWord:
                return ModeExactWord;
            case fGeneral:
            case fWeirdExactWord:
                return ModeAll;
        }
        Y_ASSERT(!"Unknown form type");
        return ModeAll;
    }

    static TFormType ModeDes2Type(TMode mode) {
        if (mode & ModeExactLemma)
            return fExactLemma;
        if (mode & ModeExactWord)
            return fExactWord;
        return fGeneral;
    }
};


class TLemmaFormsDeserializer {
public:
    static bool Deserialize(const NRichTreeProtocol::TLemma& message, TLemmaForms& lemma, TFormType formType, TLangMask& addLang);
    static void CloneButLanguage(TLemmaForms& to, const TLemmaForms& from, ELanguage newLang) {
        to = from;
        to.Language = newLang;
    }
private:
    static bool DeserializeForms(const NRichTreeProtocol::TLemma& message, TLemmaForms& lemma, TFormType& formType);

    static void DeserializeGrammar(const NRichTreeProtocol::TLemma& message, TLemmaForms& lemma);
};

static inline TWtringBuf FastCutWord(const TWtringBuf& s) {
    return s.SubStr(0, MAXWORD_LEN);
}

static inline size_t CommonPrefixLen(const TWtringBuf& s1, const TWtringBuf& s2) {
    size_t minLen = Min(s1.size(), s2.size());
    for (size_t i = 0; i < minLen; ++i)
        if (s1[i] != s2[i])
            return i;
    return minLen;
}

static void SerializeForms(const TLemmaForms& lemma, NRichTreeProtocol::TLemma& message, bool humanReadable, TFormType fType) {
    if (lemma.GetFormType() != fExactWord || fType != fExactLemma)
        fType = lemma.GetFormType();

    TVSortedForms sortedForms(lemma);

    TWtringBuf prev = FastCutWord(lemma.GetLemma());
    for (TVSortedForms::const_iterator i = sortedForms.begin(), mi = sortedForms.end(); i != mi; ++i) {
        NRichTreeProtocol::TForm* form = message.AddForms();
        Y_ASSERT(form);

        TWtringBuf formString = FastCutWord((*i)->first);

        if (formString == prev && i != sortedForms.begin())
            continue;

        if (Y_UNLIKELY(humanReadable)) {
            WideToUTF8(formString, *form->MutableForm());
        } else {
            size_t prefixLen = CommonPrefixLen(formString, prev);
            size_t backshift = prev.length() - prefixLen;
            TWtringBuf suffix = formString.SubStr(prefixLen);
            form->SetBackShift(backshift);
            WideToUTF8(suffix, *form->MutableSuffix());
        }
        prev = formString;

        if ((*i)->second.IsExact)
            form->SetMode(NOldMode___::Type2Mode(fType) | NOldMode___::ModeExactWord);
        else
            form->SetMode(NOldMode___::Type2Mode(fType));

        const TLemmaForms::TWeight& weight = (*i)->second.Weight;
        if (weight != TLemmaForms::NoWeight)
            form->SetWeight(weight);
    }
}

bool TLemmaFormsDeserializer::DeserializeForms(const NRichTreeProtocol::TLemma& message, TLemmaForms& lemma, TFormType& formType) {
    NOldMode___::TMode filtrMode = NOldMode___::Type2DesMode(formType);
    NOldMode___::TMode cumMode = filtrMode;
    const bool any = message.FormsSize() > 0;
    TUtf16String prev;
    if (any) {
        prev.reserve(MAXKEY_LEN);
        prev = lemma.GetLemma();
    }
    for (const NRichTreeProtocol::TForm& form : message.GetForms()) {
        if (form.HasForm()) {
            UTF8ToWide(form.GetForm(), prev);
        } else {
            const size_t backshift = static_cast<size_t>(form.GetBackShift());
            size_t prevLen = prev.length();
            Y_ENSURE(backshift <= prevLen, "Badly packaged forms array - backshift greater than form length");

            prevLen -= backshift;
            const auto& suffix = form.GetSuffix();
            if (backshift < suffix.size()) {
                prev.append(suffix.size() - backshift, decltype(prev)::value_type());
            }
            size_t written;
            UTF8ToWide(suffix.begin(), suffix.size(), prev.begin() + prevLen, written);
            prev.remove(prevLen + written);
        }

        Y_ENSURE(prev.length() <= MAXKEY_LEN, "Bad forms array - form is too long");

        if (!filtrMode || (form.GetMode() & filtrMode)) {
            cumMode &= form.GetMode();
            // Do not try to optimize this code with static_cast instead of operator? . static_cast here is UB.
            // See N4296 (C++14 draft), §5.2.9 [expr.static.cast], paragraph 10.
            const TLemmaForms::EMatchType isExact = form.GetMode() & NOldMode___::ModeExactWord ?
                                                    TLemmaForms::ExactMatch :
                                                    TLemmaForms::InexactMatch;
            const TLemmaForms::TWeight weight = form.HasWeight() ?
                                                form.GetWeight() :
                                                TLemmaForms::NoWeight;
            lemma.AddFormaMerge(prev, TLemmaForms::TFormWeight(weight, isExact));
        }
    }
    formType = NOldMode___::ModeDes2Type(cumMode);
    return !any || !lemma.GetForms().empty();
}

static void SerializeGrammar(const TLemmaForms& lemma, NRichTreeProtocol::TLemma& message, bool humanReadable) {
    if (humanReadable) {
        message.SetGrammar(RecodeFromYandex(CODES_UTF8, SerializeReadableGrammar(lemma.GetStemGrammar(), lemma.GetFlexGrammars())));
    } else {
        message.SetStemGramm(SerializeBinaryGrammar(lemma.GetStemGrammar()));

        for (TVector<TGramBitSet>::const_iterator i = lemma.GetFlexGrammars().begin(), mi = lemma.GetFlexGrammars().end(); i != mi; ++i) {
            message.AddFlexGramm(SerializeBinaryGrammar(*i));
        }
    }
}

static void Serialize(const TLemmaForms& lemma, NRichTreeProtocol::TLemma& message, bool humanReadable, TFormType fType) {
    WideToUTF8(FastCutWord(lemma.GetLemma()), *message.MutableLemma());
    if (humanReadable)
        message.SetTextLanguage(SerializeReadableLanguage(lemma.GetLanguage()));
    else {
        message.SetBinLanguage(lemma.GetLanguage());
    }
    message.SetBest(lemma.IsBest());
    message.SetQuality(lemma.GetQuality());
    message.SetBastard(lemma.IsBastard());
    message.SetStopWord(lemma.IsStopWord());
    message.SetStickiness(lemma.GetStickiness());

    SerializeGrammar(lemma, message, humanReadable);
    SerializeForms(lemma, message, humanReadable, fType);
}

bool TLemmaFormsDeserializer::Deserialize(const NRichTreeProtocol::TLemma& message, TLemmaForms& lemma, TFormType formType, TLangMask& addLang) {
    lemma.Lemma = UTF8ToWide(message.GetLemma());
    TLangMask lang;
    if (message.HasTextLanguage())
        lang = DeserializeReadableLanguage(message.GetTextLanguage());
    else if (message.HasBinLanguage())
        lang = static_cast<ELanguage>(message.GetBinLanguage());

    if (lang.any()) {
        lemma.Language = *lang.begin();
        lang.Reset(lemma.Language);
    } else {
        lemma.Language = LANG_UNK;
    }
    addLang = lang;
    lemma.SetBest(message.GetBest());
    lemma.Quality = message.GetQuality();
    if (lemma.Quality == TYandexLemma::QDictionary)
        lemma.Quality = message.GetBastard() ? TYandexLemma::QBastard : TYandexLemma::QDictionary; // fallback to older field

    lemma.StopWord = message.GetStopWord();
    lemma.Stickiness = static_cast<EStickySide>(message.GetStickiness());

    TLemmaFormsDeserializer::DeserializeGrammar(message, lemma);
    bool ret = TLemmaFormsDeserializer::DeserializeForms(message, lemma, formType);
    lemma.FormType = formType;
    return ret;
}

static void ParseTextGrammar(const TString& text, TGramBitSet& gram) {
    TVector<TString> names = SplitString(text, ",");
    TVector<TString>::const_iterator it;
    for (it = names.begin(); it != names.end(); ++it) {
        EGrammar code = TGrammarIndex::GetCode(*it);
        gram.SafeSet(code);
    }
}

static void DeserializeReadableGrammar(const TString& grammar, TGramBitSet& stemgram, TVector<TGramBitSet>& flexgrams) {
    if (grammar.empty())
        return;
    TString tag = grammar;

    size_t flexsep = tag.find('(');
    if (flexsep == TString::npos) {
        ParseTextGrammar(tag, stemgram);
    } else {
        ParseTextGrammar(tag.substr(0, flexsep), stemgram);
        tag = tag.substr(flexsep + 1);
        if (!tag.empty()) {
            Y_ASSERT(tag.back() == ')');
            if (tag.back() == ')')
                tag.resize(tag.length() - 1);

            TVector<TString> flexnames = SplitString(tag.c_str(), "|", 0, KEEP_EMPTY_TOKENS);
            TVector<TString>::const_iterator it;
            for (it = flexnames.begin(); it != flexnames.end(); ++it) {
                TGramBitSet grams;
                ParseTextGrammar(*it, grams);
                flexgrams.push_back(grams);
            }
        }
    }
}

void TLemmaFormsDeserializer::DeserializeGrammar(const NRichTreeProtocol::TLemma& message, TLemmaForms& lemma) {
    if (message.HasGrammar()) {
        DeserializeReadableGrammar(RecodeToYandex(CODES_UTF8, message.GetGrammar()), lemma.StemGrammar, lemma.FlexGrammars);
    } else {
        lemma.StemGrammar = DeserializeBinaryGrammar(message.GetStemGramm());

        for (size_t i = 0, mi = message.FlexGrammSize(); i != mi; ++i) {
            lemma.FlexGrammars.push_back(DeserializeBinaryGrammar(message.GetFlexGramm(i)));
        }
    }
}

namespace {
    struct TLemmPtrComparer {
        bool operator() (const TLemmaForms* a, const TLemmaForms* b) {
            return *a < *b;
        }
    };
}

static TVector<const TLemmaForms*> SortLemmas(const TWordInstance::TLemmasVector& lemms, const TWordInstance::TLemmasVector& redLemms) {
    TVector<const TLemmaForms*> res;
    res.reserve(lemms.size() + redLemms.size());
    for (TWordInstance::TLemmasVector::const_iterator i = lemms.begin(); i != lemms.end(); ++i)
        res.push_back(&*i);
    for (TWordInstance::TLemmasVector::const_iterator i = redLemms.begin(); i != redLemms.end(); ++i)
        res.push_back(&*i);
    StableSort(res.begin(), res.end(), TLemmPtrComparer());
    return res;
}

void TWordNode::Serialize(NRichTreeProtocol::TWordNode& message, bool humanReadable) const {
    TFormType fType = GetFormType();
    message.SetFormType(fType);
    WideToUTF8(GetNormalizedForm(), *message.MutableForm());

    TUtf16String attr;
    const TLemmasVector emptyLemmas{};
    const TLemmasVector* pLemms = &Lemmas;
    const TLemmasVector* pRedLemms = &RedundantLemmas;
    if (!IsLemmerWord() && Lemmas.size() == 1) {
        attr = Lemmas[0].GetLemma();
        pLemms = &emptyLemmas;
        pRedLemms = &emptyLemmas;
    }
    WideToUTF8(attr, *message.MutableAttr());

    message.SetNlpType(IsInteger() ?
            NLP_INTEGER : (IsLemmerWord() ?
                NLP_WORD : (!!attr ?
                    GuessTypeByWord(attr.data(),attr.size()) : NLP_END)));


    message.SetRevFr(RevFr);
    message.SetFormRevFr(-1);

    ::Serialize(*message.MutableLanguages(), GetLangMask(), humanReadable);
    message.SetCaseFlags(GetCaseFlags());

    EStickySide stickiness = STICK_NONE;
    bool stopWord = IsStopWord(stickiness);

    message.SetStopWord(stopWord);
    message.SetStickiness(stickiness);
    message.SetTokenCount(GetTokenCount());

    TVector<const TLemmaForms*> lms = SortLemmas(*pLemms, *pRedLemms);

    //we always want to try exact form as key
    //it will happen if we have exact lemma in lemmatized language, or exact form in not lemmatized
    //otherwise, we will need to add it manually
    TLemmaForms defaultTmpLemmaForms;
    TUtf16String lemmerNormalizedForm = NormalizeUnicode(GetNormalizedForm());
    if (lemmerNormalizedForm.size() > MAXWORD_LEN) {
         lemmerNormalizedForm = lemmerNormalizedForm.substr(0, MAXWORD_LEN);
    }
    if (!lms.empty()) {//don't add default lemma if for some reason we generated NO keys in this token
        bool hasExactKey = false;
        for (const auto& lm : lms) {
            bool isLemmatizedLanguage = NLanguageMasks::LemmasInIndex().SafeTest(lm->GetLanguage());
            if (isLemmatizedLanguage) {
                if (lm->GetLemma() == lemmerNormalizedForm) {
                    hasExactKey = true;
                    break;
                }
            } else {
                if (lm->GetForms().find(lemmerNormalizedForm) != lm->GetForms().end()) {
                    hasExactKey = true;
                    break;
                }
            }
        }
        if (!hasExactKey) {
            defaultTmpLemmaForms = TLemmaForms::MakeExactDefaultLemmaForms(TUtf16String(lemmerNormalizedForm));
            lms.push_back(&defaultTmpLemmaForms);
        }
    }

    for (TVector<const TLemmaForms*>::const_iterator i = lms.begin(); i != lms.end(); ++i) {
        NRichTreeProtocol::TLemma* lemma = message.AddLemmas();
        Y_ASSERT(lemma);
        ::Serialize(**i, *lemma, humanReadable, fType);
    }
}

static bool IgnoreNoLemmas(const TLemmaForms&) {
    return false;
}

static bool IgnoreRedundantLemmas(const TLemmaForms& lf) {
    return lf.IsBastard(TYandexLemma::QFoundling)
        && lf.GetLanguage() == ELanguage::LANG_UNK
        && lf.GetQuality() == TYandexLemma::QFoundling
        && lf.NumForms() == 1;
}

void TWordNode::Deserialize(const NRichTreeProtocol::TWordNode& message, EQtreeDeserializeMode mode) {
    LemType = message.GetNlpType() == NLP_WORD ? LEM_TYPE_LEMMER_WORD : (message.GetNlpType() == NLP_INTEGER ? LEM_TYPE_INTEGER : LEM_TYPE_NON_LEMMER);
    TFormType formType = static_cast<TFormType>(message.GetFormType());

    NormalizedForm = UTF8ToWide(message.GetForm());
    Form = NormalizedForm;
    TUtf16String attr = UTF8ToWide(message.GetAttr());

    RevFr = static_cast<long>(message.GetRevFr());

    Y_ENSURE(message.HasLanguages());
    LangMask = ::Deserialize(message.GetLanguages());

    CaseFlags = static_cast<TCharCategory>(message.GetCaseFlags());
    SetStopWord(message.GetStopWord(), static_cast<EStickySide>(message.GetStickiness()));
    TokenCount = static_cast<size_t>(message.GetTokenCount());

    const auto& filterOut = (mode == QTREE_UNSERIALIZE) ? IgnoreRedundantLemmas : IgnoreNoLemmas;
    for (size_t i = 0, mi = message.LemmasSize(); i != mi; ++i) {
        Lemmas.push_back(TLemmaForms());
        TLemmaForms& newLemma = Lemmas.back();
        TLangMask lang;
        if (!TLemmaFormsDeserializer::Deserialize(message.GetLemmas(i), newLemma, formType, lang)
            || filterOut(newLemma))
        {
            Lemmas.pop_back();
            continue;
        }
        for (ELanguage lg : lang) {
            Lemmas.push_back(TLemmaForms());
            TLemmaFormsDeserializer::CloneButLanguage(Lemmas.back(), Lemmas[Lemmas.size() - 2], lg);
        }
    }

    if (!IsLemmerWord() && message.GetNlpType() != 0) {
        Y_ENSURE(Lemmas.empty());
        Lemmas.push_back(TLemmaForms(Form, attr, LANG_UNK, false, TYandexLemma::QFoundling, formType));
        Lemmas.back().AddFormaMerge(Form);
    }
}


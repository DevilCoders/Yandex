#include <kernel/search_types/search_types.h>
#include <util/generic/utility.h>

#include "lemmer.h"
#include "language.h"
#include "lemmeraux.h"
#include <kernel/lemmer/alpha/abc.h>
#include <kernel/lemmer/dictlib/tgrammar_processing.h>

using NLemmerAux::TYandexLemmaGetter;
using NLemmerAux::TYandexLemmaSetter;
using NLemmerAux::TYandexWordformSetter;

bool operator<(const TYandexLemma& l1, const TYandexLemma& l2) {
    int r = l1.GetTextBuf().compare(l2.GetTextBuf());
    if (r)
        return r < 0;
    r = l1.GetLanguage() - l2.GetLanguage();
    if (r)
        return r < 0;
    return TYandexLemmaGetter(l1).GetRuleId() < TYandexLemmaGetter(l2).GetRuleId();
}

bool operator<(const TYandexWordform& f1, const TYandexWordform& f2) {
    int r = f1.GetText().compare(f2.GetText());
    if (r)
        return r < 0;
    r = f1.GetLanguage() - f2.GetLanguage();
    if (r)
        return r < 0;
    return f1.GetStemGram() < f2.GetStemGram(); // сравниваем прям указатели - по фиг
}

size_t FillUniqLemmas(TWUniqueLemmaArray& uniques, const TYandexLemma data[], size_t len) {
    uniques.resize(0);
    uniques.reserve(len);
    const TYandexLemma* dataend = data + len;
    for (const TYandexLemma* p1 = data; p1 != dataend; p1++) {
        // look at the end - chances are that the last lemma is the same
        TVector<const TYandexLemma*>::reverse_iterator it = uniques.rbegin();
        while (true) {
            if (it == uniques.rend()) {
                uniques.push_back(p1); // this lemma is unique
                break;
            }
            const TYandexLemma* p2 = *(it++);
            if (p1->GetTextBuf() == p2->GetTextBuf())
                break; // got a duplicate - discard
        }
    }
    return uniques.size();
}

TYandexLemma::TYandexLemma()
    : LemmaLen(0)
    , SuffixLen(0)
    , Language(LANG_UNK)
    , RuleId(0)
    , Quality(0)
    , Depth(0)
    , StemGram("")
    , GramNum(0)
    , LemmaPrefLen(0)
    , PrefLen(0)
    , FlexLen(0)
    , FormInitialLen(0)
    , FormNormalizedLen(0)
    , FormConvertedLen(0)
    , Flags(0)
    , TokenPos(0)
    , TokenSpan(1)
    , ParadigmId(0)
    , MinDistortion((size_t)(-1))
    , Weight(0.0)
    , LanguageVersion(1)
{
    LemmaText[0] = 0;
    FormInitial[0] = 0;
    FormNormalized[0] = 0;
    AdditionGram[0] = 0;
}

static const TLanguage* GetLang(const TYandexLemma* lm) {
    return NLemmer::GetLanguageByIdAnyway(lm->GetLanguage());
}

NLemmer::TLllLevel TYandexLemma::LooksLikeLemma() const {
    return GetLang(this)->LooksLikeLemma(*this);
}

TAutoPtr<NLemmer::TGrammarFiltr> TYandexLemma::LooksLikeLemmaExt() const {
    return GetLang(this)->LooksLikeLemmaExt(*this);
}

TAutoPtr<NLemmer::TFormGenerator> TYandexLemma::Generator(const NLemmer::TGrammarFiltr* filter) const {
    return GetLang(this)->Generator(*this, filter);
}

size_t TYandexLemma::GetFormsCount() const {
    TAutoPtr<NLemmer::TFormGenerator> generator = Generator();
    return generator.Get() ? generator->FormsCount() : 0;
}

bool TYandexLemma::HasStemGram(EGrammar gram) const {
    return NTGrammarProcessing::HasGram(GetStemGram(), gram);
}

bool TYandexLemma::HasFlexGram(EGrammar gram) const {
    bool hasGram = false;
    for (size_t flexNum = 0; flexNum < FlexGramNum(); ++flexNum) {
        if (hasGram) {
            break;
        }
        hasGram |= NTGrammarProcessing::HasGram(GetFlexGram()[flexNum], gram);
    }
    return hasGram;
}

bool TYandexLemma::HasGram(EGrammar gram) const {
    if (HasStemGram(gram)) {
        return true;
    }

    if (HasFlexGram(gram)) {
        return true;
    }

    return false;
}

void TYandexLemmaSetter::SetLemmaInfo(ui32 ruleId, size_t prefLen, size_t flexLen, const char* stemGram, size_t paradigmId, size_t lemmaPrefLen) {
    Lemma.RuleId = ruleId;
    Lemma.PrefLen = prefLen;
    Lemma.FlexLen = flexLen;
    Lemma.StemGram = stemGram;
    Lemma.ParadigmId = paradigmId;
    Lemma.LemmaPrefLen = lemmaPrefLen;
}

void TYandexLemmaSetter::SetLemma(const wchar16* lemmaText, size_t lemmaLen, ui32 ruleId, size_t prefLen, size_t flexLen, const char* stemGram) {
    SetLemmaText(lemmaText, lemmaLen);
    SetLemmaInfo(ruleId, prefLen, flexLen, stemGram);
}

void TYandexLemmaSetter::ResetLemmaToForm(bool convert) {
    wchar16 curLemmaText[MAXWORD_BUF];
    size_t curLemmaLen = Lemma.LemmaLen;
    memcpy(curLemmaText, Lemma.LemmaText, curLemmaLen * sizeof(wchar16));

    if (convert) {
        NLemmer::TConvertRet ret = GetLang(&Lemma)->Normalize(Lemma.FormNormalized, Lemma.FormNormalizedLen,
                                                              Lemma.LemmaText, MAXWORD_BUF - 1, NLemmer::TAlphabetWordNormalizer::ModeConvertNormalized());
        if (ret.Valid) {
            Lemma.LemmaLen = ret.Length;
            Lemma.LemmaText[Lemma.LemmaLen] = 0;
        } else
            convert = false;
    }

    if (!convert) {
        Lemma.LemmaLen = Min(Lemma.FormNormalizedLen, (size_t)MAXWORD_BUF - 1);
        memcpy(Lemma.LemmaText, Lemma.FormNormalized, Lemma.LemmaLen * sizeof(wchar16));
        Lemma.LemmaText[Lemma.LemmaLen] = 0;
    }

    if (curLemmaLen != Lemma.LemmaLen || memcmp(curLemmaText, Lemma.LemmaText, curLemmaLen * sizeof(wchar16)) != 0)
        Lemma.Quality |= TYandexLemma::QOverrode;
}

void TYandexLemmaSetter::SetInitialForm(const wchar16* form, size_t len) {
    Lemma.FormInitialLen = Min(len, (size_t)TYandexLemma::InitialBufSize - 1);
    memcpy(Lemma.FormInitial, form, Lemma.FormInitialLen * sizeof(wchar16));
    Lemma.FormInitial[Lemma.FormInitialLen] = 0;
}

void TYandexLemmaSetter::SetNormalizedForm(const wchar16* form, size_t len) {
    Lemma.FormNormalizedLen = Min(len, (size_t)MAXWORD_BUF - 1);
    memcpy(Lemma.FormNormalized, form, Lemma.FormNormalizedLen * sizeof(wchar16));
    Lemma.FormNormalized[Lemma.FormNormalizedLen] = 0;
}

void TYandexLemmaSetter::SetForms(const wchar16* form, size_t len) {
    Lemma.FormInitialLen = NLemmerAux::Decompose(form, len, Lemma.FormInitial, TYandexLemma::InitialBufSize).Length;
    if (Lemma.FormInitialLen > TYandexLemma::InitialBufSize - 1)
        Lemma.FormInitialLen = TYandexLemma::InitialBufSize - 1;
    Lemma.FormInitial[Lemma.FormInitialLen] = 0;

    Lemma.FormNormalizedLen = NLemmer::GetAlphaRulesUnsafe(LANG_UNK)->Compose(Lemma.FormInitial, Lemma.FormInitialLen, Lemma.FormNormalized, MAXWORD_BUF).Length;
    if (Lemma.FormNormalizedLen > MAXWORD_BUF - 1)
        Lemma.FormNormalizedLen = MAXWORD_BUF - 1;
    Lemma.FormNormalized[Lemma.FormNormalizedLen] = 0;
}

static void AddSuff(wchar16* text, size_t& textLen, const wchar16* suffix, const size_t sufLen) {
    memcpy(text + textLen, suffix, sufLen * sizeof(wchar16));
    textLen = textLen + sufLen;
    text[textLen] = 0;
}

static void RemoveSuffix(wchar16* text, size_t& textLen, const size_t sufLen) {
    Y_ASSERT((!textLen && !sufLen) || (textLen > sufLen));
    textLen -= sufLen;
    text[textLen] = 0;
}

bool TYandexLemmaSetter::AddSuffix(const wchar16* suffix, size_t len) {
    RemoveSuffix(Lemma.LemmaText, Lemma.LemmaLen, Lemma.SuffixLen);
    RemoveSuffix(Lemma.FormInitial, Lemma.FormInitialLen, Lemma.SuffixLen);
    RemoveSuffix(Lemma.FormNormalized, Lemma.FormNormalizedLen, Lemma.SuffixLen);
    Lemma.SuffixLen = 0;
    if (Lemma.GetTextLength() + len >= MAXWORD_BUF || Lemma.GetInitialFormLength() + len >= TYandexLemma::InitialBufSize || Lemma.GetNormalizedFormLength() + len >= MAXWORD_BUF) {
        return false;
    }
    AddSuff(Lemma.LemmaText, Lemma.LemmaLen, suffix, len);
    AddSuff(Lemma.FormInitial, Lemma.FormInitialLen, suffix, len);
    AddSuff(Lemma.FormNormalized, Lemma.FormNormalizedLen, suffix, len);
    Lemma.SuffixLen = len;
    return true;
}

void TYandexLemmaSetter::Clear() {
    Lemma.LemmaText[0] = 0;
    Lemma.LemmaLen = 0;
    Lemma.SuffixLen = 0;
    Lemma.Language = LANG_UNK;
    Lemma.RuleId = 0;
    Lemma.Quality = 0;
    Lemma.StemGram = "";
    Lemma.GramNum = 0;
    Lemma.LemmaPrefLen = 0;
    Lemma.PrefLen = 0;
    Lemma.FlexLen = 0;
    Lemma.FormInitial[0] = 0;
    Lemma.FormInitialLen = 0;
    Lemma.FormNormalized[0] = 0;
    Lemma.FormNormalizedLen = 0;
    Lemma.Flags = 0;
    Lemma.TokenPos = 0;
    Lemma.TokenSpan = 1;
    Lemma.AdditionGram[0] = 0;
    Lemma.Depth = 0;
}

void TYandexWordformSetter::SetNormalizedForm(const wchar16* form, size_t totalLen, ELanguage language, size_t stemLen, size_t prefixLen, size_t suffixLen) {
    Form.Language = language;
    Form.StemLen = stemLen;
    Form.PrefixLen = prefixLen;
    Form.SuffixLen = suffixLen;

    Form.Text.assign(form, totalLen);
}

void TYandexWordformSetter::SetNormalizedForm(const TUtf16String& form, ELanguage language, size_t stemLen, size_t prefixLen, size_t suffixLen) {
    Form.Language = language;
    Form.StemLen = stemLen;
    Form.PrefixLen = prefixLen;
    Form.SuffixLen = suffixLen;
    Form.Text = form;
}

void TYandexWordformSetter::AddSuffix(const wchar16* Suffix, size_t len) {
    Form.Text.append(Suffix, len);
    Form.SuffixLen = len;
}

void TYandexWordformSetter::Clear() {
    Form.Text.clear();
    Form.Language = LANG_UNK;
    Form.StemLen = 0;
    Form.PrefixLen = 0;
    Form.SuffixLen = 0;
    Form.StemGram = nullptr;
    Form.FlexGram.clear();
}

TYandexWordform::TYandexWordform()
    : Language(LANG_UNK)
    , StemLen(0)
    , SuffixLen(0)
    , StemGram(nullptr)
{
}

#include "numeral.h"
#include "gramfeatures.h"
#include "inflector.h"
#include "ylemma.h"

#include <util/generic/singleton.h>

namespace NInfl {


using NSpike::TGrammarBunch;


struct TNumFlexForm {
    const TUtf16String* Flexion;
    TGramBitSet Grammems;

    TNumFlexForm()
        : Flexion(nullptr)
        , Grammems()
    {}

    TNumFlexForm(const TGramBitSet& gram)
        : Flexion(nullptr)
        , Grammems(gram)
    {}

    TNumFlexForm(const TUtf16String& flex, const TGramBitSet& gram)
        : Flexion(&flex)
        , Grammems(gram)
    {}
};

// TFormOps<TNumFlexForm> specialization
template <>
struct TFormOps<TNumFlexForm> {

    static bool IsValid(const TNumFlexForm& form) {
        return form.Flexion != nullptr;
    }

    static inline TGramBitSet ExtractGrammems(const TNumFlexForm& form) {
        return form.Grammems;
    }

    static inline size_t CommonPrefixSize(const TNumFlexForm& /*form1*/, const TNumFlexForm& /*form2*/) {
        return 0;   // not necessary
    }

    static TString DebugString(const TNumFlexForm&) {
        return TString();    // TODO
    }
};

class TNumFlexParadigm {
public:
    TNumFlexParadigm(size_t maxFlexLen, size_t minFlexLen = 1)
        : MaxFlexLen(maxFlexLen), MinFlexLen(minFlexLen)
    {
    }

    virtual ~TNumFlexParadigm() {
    }

    void AddForm(const TWtringBuf& text, const TGramBitSet& grammems) {
        for (size_t len = MinFlexLen; len <= MaxFlexLen && len <= text.size(); ++len) {
            TUtf16String flex = ::ToWtring(text.Last(len));
            Forms[flex].insert(grammems);
        }
        TUtf16String norm = Normalize(text);
        if (!norm.empty())
            NormForms[norm].insert(grammems);
    }

    TWtringBuf DefaultNormalize(const TWtringBuf& flex) const {
        // default normalization: take last 2 characters
        return flex.Last(2);
    }

    virtual TUtf16String Normalize(const TWtringBuf& flex) const {
        return ::ToWtring(DefaultNormalize(flex));
    }

    const TGrammarBunch* Recognize(const TWtringBuf& flexion) const {
        TFlexMap::const_iterator form = Forms.find(flexion.Last(MaxFlexLen));
        return form != Forms.end() ? &(form->second) : nullptr;
    }

    bool Inflect(const TWtringBuf& flexion, const TGramBitSet& requested,
                 TUtf16String& restext, TGramBitSet& resgram) const;

private:
    inline bool InflectForm(TNumFlexForm& form, const TGramBitSet& requested) const;

private:
    size_t MaxFlexLen, MinFlexLen;

    typedef TMap<TUtf16String, TGrammarBunch, TLess<TUtf16String> > TFlexMap;
    TFlexMap Forms;       // all flexions (of size 1 to MaxFlexLen)
    TFlexMap NormForms;    // only normalized flexions (e.g. of size 2)

    friend class TNumFlexIterator;
};


class TNumFlexIterator {
public:
    TNumFlexIterator(const TNumFlexParadigm& numflex)
        : NumFlex(&numflex)
    {
        Restart();
    }

    bool Restart() {
        CurFlex = NumFlex->NormForms.begin();
        Y_ASSERT(!CurFlex->second.empty());
        CurGram = CurFlex->second.begin();
        return Ok();
    }

    bool Ok() const {
        return CurFlex != NumFlex->NormForms.end() &&
               CurGram != CurFlex->second.end();
    }

    void operator++() {
        ++CurGram;
        if (CurGram == CurFlex->second.end()) {
            ++CurFlex;
            if (CurFlex != NumFlex->NormForms.end()) {
                Y_ASSERT(!CurFlex->second.empty());
                CurGram = CurFlex->second.begin();
            }
        }
    }

    TNumFlexForm operator* () const {
        return TNumFlexForm(CurFlex->first, *CurGram);
    }

private:
    const TNumFlexParadigm* NumFlex;
    TNumFlexParadigm::TFlexMap::const_iterator CurFlex;
    TGrammarBunch::const_iterator CurGram;
};


inline bool TNumFlexParadigm::InflectForm(TNumFlexForm& form, const TGramBitSet& requested) const {
    TInflector<TNumFlexForm, TNumFlexIterator> infl(form, TNumFlexIterator(*this));
    return infl.InflectSupported(requested);
}

bool TNumFlexParadigm::Inflect(const TWtringBuf& flexion, const TGramBitSet& requested,
                               TUtf16String& restext, TGramBitSet& resgram) const {
    const TGrammarBunch* forms = Recognize(flexion);

    TNumFlexForm best;
    size_t bestNormal = 0;
    if (forms != nullptr)
        for (TGrammarBunch::const_iterator it = forms->begin(); it != forms->end(); ++it) {
            TNumFlexForm cur(*it);
            if (InflectForm(cur, requested) && cur.Flexion != nullptr) {
                size_t curNormal = (cur.Grammems & DefaultFeatures().NormalMutableSet).count();
                if (best.Flexion == nullptr || bestNormal < curNormal) {
                    best = cur;
                    bestNormal = curNormal;
                }
/*
                else if (*best.Flexion != *cur.Flexion)     // ambiguity
                    return false;
*/
            }
        }

    if (best.Flexion == nullptr) {
        TNumFlexForm cur;
        if (InflectForm(cur, requested) && cur.Flexion != nullptr)
            best = cur;
    }

    if (best.Flexion != nullptr) {
        restext = *best.Flexion;
        resgram = best.Grammems;
        return true;
    } else
        return false;
}




class TNumFlexByk {
public:
    TNumFlexByk()
        : Language(LANG_UNK)
    {
    }

    virtual ~TNumFlexByk() {
    }

    virtual bool Split(const TWtringBuf& text, TWtringBuf& numeral, TWtringBuf& delim, TWtringBuf& flexion) const;

    virtual const TNumFlexParadigm* Recognize(const TWtringBuf& /*numeral*/, const TWtringBuf& /*flex*/) const {
        return nullptr;
    }

protected:
    virtual void AddForms(const TUtf16String& numeral, TNumFlexParadigm* dst, const TGramBitSet& filter);
    inline void AddNumeralForms(const TUtf16String& numeral, TNumFlexParadigm* dst) {
        AddForms(numeral, dst, TGramBitSet(gAdjNumeral));
    }

    ELanguage Language;
};


void TNumFlexByk::AddForms(const TUtf16String& numeral, TNumFlexParadigm* dst, const TGramBitSet& filter) {
    TVector<TYandexLemma> lemmas;
    TUtf16String text;

    NLemmer::AnalyzeWord(numeral.data(), numeral.size(), lemmas, Language);
    for (size_t i = 0; i < lemmas.size(); ++i) {
        TGramBitSet grammems;
        NSpike::ToGramBitset(lemmas[i].GetStemGram(), grammems);
        if (!grammems.HasAll(filter))
            continue;

        TAutoPtr<NLemmer::TFormGenerator> gen = lemmas[i].Generator();
        for (NLemmer::TFormGenerator& kit = *gen; kit.IsValid(); ++kit) {
            grammems = TYemmaInflector::Grammems(*kit);
            //filter excessive unnecessary forms (currently, only obsolete)
            if (grammems.Has(gObsolete))
                continue;
            kit->ConstructText(text);
            dst->AddForm(text, grammems);
        }
        break;
    }
}

bool TNumFlexByk::Split(const TWtringBuf& text, TWtringBuf& numeral, TWtringBuf& delim, TWtringBuf& flexion) const {
    size_t i = text.size();
    for (; i > 0; --i)
        if (!IsAlpha(text[i-1]))
            break;

    TWtringBuf f = TWtringBuf(text.data() + i, text.data() + text.size());
    if (f.empty())
        return false;

    for (; i > 0; --i)
        if (!IsPunct(text[i-1]))
            break;

    TWtringBuf d = TWtringBuf(text.data() + i, f.data());
    for (; i > 0; --i)
        if (!IsCommonDigit(text[i-1]))
            break;

    TWtringBuf n = TWtringBuf(text.data() + i, d.data());
    if (!n.empty()) {
        numeral = n;
        delim = d;
        flexion = f;
        return true;
    } else
        return false;
}



class TNumFlexParadigmRus: public TNumFlexParadigm {
public:
    TNumFlexParadigmRus()
        : TNumFlexParadigm(4, 1)
    {
    }

    TUtf16String Normalize(const TWtringBuf& flex) const override {
        TWtringBuf res = DefaultNormalize(flex);
        if (!res.empty() && res[0] == 0x44c) // 'ь'
            res.Skip(1);
        return ::ToWtring(res);
    }
};

class TNumLetniyParadigmRus: public TNumFlexParadigm {
public:
    TNumLetniyParadigmRus()
        : TNumFlexParadigm(6, 5)
    {
    }

    TUtf16String Normalize(const TWtringBuf& flex) const override {
        TWtringBuf res(flex);
        while (!res.empty() && res[0] != 0x43b)  // 'л' (ищем начало корня "летн")
            res.Skip(1);
        return ::ToWtring(res);
    }
};

class TNumFlexBykRus: public TNumFlexByk {
public:
    TNumFlexBykRus();
    const TNumFlexParadigm* Recognize(const TWtringBuf& numeral, const TWtringBuf& flexion) const override;
private:
    TNumFlexParadigmRus Perviy;     // 1-ый, 5-тый, 10-й
    TNumFlexParadigmRus Vtoroy;     // 2-ой, 7-мой, 40-вой
    TNumFlexParadigmRus Tretiy;     // 3-ий
    TNumLetniyParadigmRus Stoletniy;  // 2-летний, 7-летний, 40-летний, 100-летний
};


TNumFlexBykRus::TNumFlexBykRus()
{
    Language = LANG_RUS;
    AddNumeralForms(u"первый", &Perviy);
    AddNumeralForms(u"второй", &Vtoroy);
    AddNumeralForms(u"третий", &Tretiy);

    AddNumeralForms(u"четвертый", &Perviy);
    AddNumeralForms(u"шестой", &Vtoroy);
    AddNumeralForms(u"седьмой", &Vtoroy);

    AddNumeralForms(u"сороковой", &Vtoroy);

    AddNumeralForms(u"тысячный", &Perviy);

    TGramBitSet flt(gAdjective, gFull);
    AddForms(u"двухлетний", &Stoletniy, flt);
    AddForms(u"семилетний", &Stoletniy, flt);
    AddForms(u"сорокалетний", &Stoletniy, flt);
    AddForms(u"столетний", &Stoletniy, flt);
    AddForms(u"тысячелетний", &Stoletniy, flt);
}

const TNumFlexParadigm* TNumFlexBykRus::Recognize(const TWtringBuf& numeral, const TWtringBuf& flexion) const {

    // 19-letniy
    if (flexion.size() >= 5 && Stoletniy.Recognize(flexion))
        return &Stoletniy;

    size_t L = numeral.size();
    if (L <= 0)
        return nullptr;

    // 10-iy, 11-iy, 12, ... , 19-iy
    if (L >= 2 && numeral[L-2] == '1' && ::IsCommonDigit(numeral[L-1]))
        return &Perviy;

    switch (numeral[L-1]) {
        case '0':
            if (L >= 2 && ::IsCommonDigit(numeral[L-2]) && numeral[L-2] != '4')
                return &Perviy;   // 10-iy, 20-iy, 100-iy, 1000-iy, ...
            else
                return &Vtoroy;   // 0-oy, 40-oy
        case '1':
        case '4':
        case '5':
        case '9':
            return &Perviy;

        case '2':
        case '6':
        case '7':
        case '8':
            return &Vtoroy;

        case '3':
            return &Tretiy;

        default:
            return nullptr;
    }
}


class TNumFlexLemmer {
public:
    // should be created via Singleton
    TNumFlexLemmer();

    static const TNumFlexByk& Default() {
        return *(Singleton<TNumFlexLemmer>()->DefaultByk);
    }

    static const TNumFlexByk& Byk(const TLangMask& langMask) {
        const TNumFlexLemmer* lemmer = Singleton<TNumFlexLemmer>();
        for (ELanguage lang : langMask) {
            TMap<ELanguage, TSimpleSharedPtr<TNumFlexByk> >::const_iterator it = lemmer->Byks.find(lang);
            if (it != lemmer->Byks.end())
                return *(it->second);
        }
        return Default();
    }

private:
    TSimpleSharedPtr<TNumFlexByk> DefaultByk;
    TMap<ELanguage, TSimpleSharedPtr<TNumFlexByk> > Byks;
};

TNumFlexLemmer::TNumFlexLemmer()
    : DefaultByk(new TNumFlexByk)
{
    Byks[LANG_UNK] = DefaultByk;
    Byks[LANG_RUS] = new TNumFlexBykRus;
}



TNumeralAbbr::TNumeralAbbr(const TLangMask& langMask, const TWtringBuf& wordText)
    : Prefix(wordText)
    , Paradigm(nullptr)
{
    const TNumFlexByk& byk = TNumFlexLemmer::Byk(langMask);
    if (byk.Split(wordText, Numeral, Delim, Flexion)) {
        Paradigm = byk.Recognize(Numeral, Flexion);
        Prefix = TWtringBuf(wordText.data(), Numeral.data());
    }
}


bool TNumeralAbbr::Inflect(const TGramBitSet& grammems, TUtf16String& restext, TGramBitSet* resgram) const {
    TGramBitSet rgram;
    TUtf16String newflex;
    if (Paradigm != nullptr && Paradigm->Inflect(Flexion, grammems, newflex, rgram)) {
        if (resgram != nullptr)
            *resgram = rgram;
        ConstructText(newflex, restext);
        return true;
    } else
        return false;
}

void TNumeralAbbr::ConstructText(const TWtringBuf& newflex, TUtf16String& text) const {
    text.reserve(Prefix.size() + Numeral.size() + Delim.size() + newflex.size());
    text.assign(Prefix.data(), Prefix.size());
    text.append(Numeral.data(), Numeral.size());
    text.append(Delim.data(), Delim.size());
    text.append(newflex.data(), newflex.size());
}


}   // namespace NInfl


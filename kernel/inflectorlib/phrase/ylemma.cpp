#include "ylemma.h"
#include "inflector.h"

#include <kernel/search_types/search_types.h>
#include <google/protobuf/stubs/substitute.h>
#include <library/cpp/charset/wide.h>
using ::google::protobuf::strings::Substitute;


namespace NInfl {

// TFormOps<TWordformKit> specialization
template <>
struct TFormOps<TWordformKit> {
    static bool IsValid(const TWordformKit& /*form*/) {
        return true;
    }

    static inline TGramBitSet ExtractGrammems(const TWordformKit& form) {
        return TYemmaInflector::Grammems(form);
    }

    static inline size_t CommonPrefixSize(const TWordformKit& form1, const TWordformKit& form2) {
        return form1.CommonPrefixLength(form2);
    }

    static TString DebugString(const TWordformKit& form) {
        TStringStream out;
        TUtf16String text;
        form.ConstructText(text);
        out << text << "::" << ExtractGrammems(form).ToString(",");
        return out.Str();
    }
};

class TWordformKitIterator {
public:
    TWordformKitIterator(const TYandexLemma* lemma)
        : Lemma(lemma)
    {
    }

    bool Restart() {
        Generator = Lemma->Generator();
        return Ok();
    }

    bool Ok() const {
        Y_ASSERT(Generator.Get() != nullptr);
        return Generator->IsValid();
    }

    void operator++() {
        Y_ASSERT(Generator.Get() != nullptr);
        ++(*Generator);
    }

    const TWordformKit& operator* () const {
        Y_ASSERT(Generator.Get() != nullptr);
        return **Generator;
    }

private:
    const TYandexLemma* Lemma;
    TAutoPtr<NLemmer::TFormGenerator> Generator;
};

bool TYemmaInflector::Inflect(const TGramBitSet& requested, bool strict) {
    TInflector<TWordformKit, TWordformKitIterator> infl(CurrentBest, TWordformKitIterator(Original));
    return infl.Inflect(requested, strict);
}

bool TYemmaInflector::InflectSupported(const TGramBitSet& requested) {
    TInflector<TWordformKit, TWordformKitIterator> infl(CurrentBest, TWordformKitIterator(Original));
    return infl.InflectSupported(requested);
}

void TYemmaInflector::ApplyTo(TYandexLemma& word) const {
    wchar16 buffer[MAXWORD_BUF];
    TWtringBuf text = CurrentBest.ConstructText(buffer, MAXWORD_BUF);
    NLemmerAux::TYandexLemmaSetter setter(word);
    setter.SetNormalizedForm(text.data(), text.size());
    const char* flexgr = CurrentBest.GetFlexGram();
    setter.SetFlexGrs(&flexgr, 1);
}

TString TYemmaInflector::DebugString() const {
    TYandexLemma copy = *Original;
    ApplyTo(copy);
    return Substitute("$0 ($1 + $2)",
                      WideToChar(copy.GetNormalizedForm(), copy.GetNormalizedFormLength(), CODES_YANDEX),
                      TGramBitSet::FromBytes(copy.GetStemGram()).ToString(","),
                      TGramBitSet::FromBytes(copy.GetFlexGram()[0]).ToString(","));
}




void TCollocation::AddDependency(size_t from, size_t to, const TGramBitSet& agreeTo,
                                 const TGramBitSet& enforceFrom, const TGramBitSet& enforceTo) {
    Agreements.emplace_back();
    Agreements.back().From = from;
    Agreements.back().To = to;
    Agreements.back().AgreeTo = agreeTo;
    Agreements.back().EnforceFrom = enforceFrom;
    Agreements.back().EnforceTo = enforceTo;
}


bool TCollocation::Inflect(size_t from, const TGramBitSet& requested) {
    if (!Words[from].Inflect(requested, false))
        return false;

    TGramBitSet mainGrammems = Words[from].Grammems();
    TGramBitSet enforceFrom;

    for (size_t i = 0; i < Agreements.size(); ++i) {
        const TDependency& dep = Agreements[i];
        if (dep.From == from) {
            TGramBitSet agreedTo = (mainGrammems & dep.AgreeTo) | dep.EnforceTo;
            if (!Words[dep.To].InflectSupported(agreedTo))
                return false;
            enforceFrom |= dep.EnforceFrom;
        }
    }

    // if there is enforce-from grammems, try modifying original word again with new requirement
    return enforceFrom.none() || Words[from].Inflect(requested | enforceFrom, false);
}

TString TCollocation::DebugString() const {
    if (Words.empty())
        return TString();
    TString res = Words[0].DebugString();
    for (size_t i = 1; i < Words.size(); ++i) {
        res += " ";
        res += Words[i].DebugString();
    }
    return res;
}


}   // NInfl

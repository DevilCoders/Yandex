#include "automorphology.h"

#include <library/cpp/archive/yarchive.h>
#include <kernel/lemmer/core/lemmeraux.h>
#include <library/cpp/digest/md5/md5.h>

#include <util/charset/wide.h>
#include <util/generic/singleton.h>
#include <util/system/spinlock.h>

constexpr char TSimpleCompactLister::PREFIX_SUFFIX_DELIMITER;
constexpr size_t TSimpleCompactLister::PREFIX_BITS;
constexpr size_t TSimpleCompactLister::STEM_BITS;
constexpr size_t TSimpleCompactLister::PARADIGM_BITS;

using NAutomorphology::TParadigm;

namespace NAutomorphology {
    TParadigm SingleWordParadigm(const TUtf16String& text) {
        return TParadigm(1, TParadigmWord(text, 0, 0));
    }
}

TSimpleCompactLister::TSimpleCompactLister(IInputStream* wordsIn, IInputStream* suffixesIn, IInputStream* paradigmsIn) {
    TUtf16String line;

    WordParadigms.Init(TBlob::FromStream(*wordsIn));

    while (suffixesIn->ReadLine(line)) {
        size_t delimiterPos = line.find(PREFIX_SUFFIX_DELIMITER);
        Suffixes.push_back(line.substr(0, delimiterPos));
        if (delimiterPos != TString::npos) {
            Prefixes.push_back(line.substr(delimiterPos + 1));
        } else {
            Prefixes.push_back(Default<TUtf16String>());
        }
    }

    {
        ui32 n;
        while (paradigmsIn->Read(&n, sizeof(ui32))) {
            SuffixParadigms.push_back(n);
        }
    }
}

bool TSimpleCompactLister::IsWord(const TUtf16String& word) const {
    return WordParadigms.Find(word.c_str(), word.size());
}

TParadigm TSimpleCompactLister::GetForms(const TUtf16String& word) const {
    ui64 p;
    if (!WordParadigms.Find(word.c_str(), word.size(), &p)) {
        return NAutomorphology::SingleWordParadigm(word);
    }

    //data format: prefix[16] stem[16] paradigm[32]
    ui32 prefixLength = (p >> (STEM_BITS + PARADIGM_BITS));
    ui32 stemLength = (p >> PARADIGM_BITS) % (static_cast<ui64>(1) << PREFIX_BITS);
    ui32 paradigmNumber = p % (static_cast<ui64>(1) << PARADIGM_BITS);
    TWtringBuf stem = TWtringBuf(word).substr(prefixLength, stemLength);

    /*TVector<TUtf16String> result;
    result.push_back(word);*/
    TParadigm result;
    result.emplace_back(word, prefixLength, word.size() - stem.size() - prefixLength);

    Y_ASSERT(paradigmNumber + 1 < SuffixParadigms.size());

    for (size_t suffix = SuffixParadigms[paradigmNumber]; suffix < SuffixParadigms[paradigmNumber + 1]; ++suffix) {
        Y_ASSERT(suffix < SuffixParadigms.size());
        Y_ASSERT(SuffixParadigms[suffix] < Suffixes.size());
        const TUtf16String& prefixText = Prefixes[SuffixParadigms[suffix]];
        const TUtf16String& suffixText = Suffixes[SuffixParadigms[suffix]];
        result.emplace_back(prefixText + stem + suffixText, prefixText.size(), suffixText.size());
    }
    return result;
}

TString TSimpleCompactLister::Fingerprint() const {
    static TAdaptiveLock lock;
    if (!DataFingerprint.size()) {
        TGuard guard(lock);
        if (!DataFingerprint.size()) {
            char buf[25];
            MD5::Data(static_cast<const unsigned char*>(WordParadigms.Data().Data()), WordParadigms.Data().Size(), buf);
            DataFingerprint = TString(buf);
        }
    }
    return DataFingerprint;
}

TSimpleSharedPtr<TAutomorphologyLanguage> TAutomorphologyLanguage::LoadLanguage(ELanguage id, IInputStream* wordsIn, IInputStream* suffixesIn, IInputStream* paradigmsIn) {
    if (!LoadedLangs.contains(id)) {
        LoadedLangs[id] = new TAutomorphologyLanguage(id, false, wordsIn, suffixesIn, paradigmsIn);
    }
    return LoadedLangs[id];
}

TSimpleSharedPtr<TAutomorphologyLanguage> TAutomorphologyLanguage::LoadLanguage(ELanguage id, const unsigned char* data, size_t dataSize) {
    static TAdaptiveLock lock;
    if (!LoadedLangs.contains(id)) {
        TGuard guard(lock);
        if (!LoadedLangs.contains(id)) {
            TArchiveReader reader(TBlob::NoCopy(data, dataSize));
            TString wordsData(reader.ObjectByKey("/trie")->ReadAll());
            TString suffixesData(reader.ObjectByKey("/suffixes")->ReadAll());
            TString paradigmsData(reader.ObjectByKey("/paradigms")->ReadAll());
            TStringInput wordsIn(wordsData);
            TStringInput suffixesIn(suffixesData);
            TStringInput paradigmsIn(paradigmsData);
            return LoadLanguage(id, &wordsIn, &suffixesIn, &paradigmsIn);
        }
    }
    return LoadedLangs[id];
}

TSimpleSharedPtr<TAutomorphologyLanguage> TAutomorphologyLanguage::LoadLanguage(ELanguage id, const TString& path) {
    if (!LoadedLangs.contains(id)) {
        TFileInput wordsIn(path + "trie"), suffixesIn(path + "suffixes"), paradigmsIn(path + "paradigms");
        return LoadLanguage(id, &wordsIn, &suffixesIn, &paradigmsIn);
    }
    return LoadedLangs[id];
}

TAutomorphologyLanguage::TAutomorphologyLanguage(ELanguage id, bool dontRegister, IInputStream* wordsIn, IInputStream* suffixesIn, IInputStream* paradigmsIn)
    : TLanguage(id, dontRegister, 999)
    , SimpleLister(wordsIn, suffixesIn, paradigmsIn)
{
}

size_t TAutomorphologyLanguage::RecognizeInternal(const wchar16* word, size_t length, TWLemmaArray& out, const NLemmer::TRecognizeOpt& opt) const {
    if (!opt.Accept.SafeTest(NLemmer::EAccept::AccBastard)) {
        return 0;
    }
    out.push_back(TYandexLemma());
    NLemmerAux::TYandexLemmaSetter set(out.back());
    static const char fictiveGram[] = {NTGrammarProcessing::tg2ch(gSubstantive), 0};
    set.SetLemma(word, length, 0, length, 0, fictiveGram);
    if (SimpleLister.IsWord(TUtf16String(word))) {
        set.SetQuality(TYandexLemma::QBastard | TYandexLemma::QAutomorphology | TYandexLemma::QFix);
    } else {
        set.SetQuality(TYandexLemma::QFoundling | TYandexLemma::QFix);
    }
    set.SetLanguageVersion(Version);
    return 1;
}

class TAutomorphologyFormIterator: public IWordformIterator {
private:
    TVector<TYandexLemma> Lemmas;
    size_t Index = 0;
    THolder<TWordformKit> CurKit;

    const TWordformKit& GetValue() const override {
        return *CurKit;
    }

public:
    TAutomorphologyFormIterator(const TParadigm& paradigm) {
        static const char fictiveGram[] = {NTGrammarProcessing::tg2ch(gSubstantive), 0};
        Lemmas.resize(paradigm.size());
        for (size_t i = 0; i < paradigm.size(); ++i) {
            NLemmerAux::TYandexLemmaSetter setter(Lemmas[i]);
            const TUtf16String& word = paradigm[i].Text;
            setter.SetLemma(word.c_str(), word.size(), 0, paradigm[i].PrefixLength, paradigm[i].SuffixLength, fictiveGram);
            setter.SetNormalizedForm(word.c_str(), word.size());
        }
        if (IsValid()) {
            CurKit = MakeHolder<TWordformKit>(Lemmas[Index], 0);
        }
    }

    void operator++() override {
        ++Index;
        if (IsValid()) {
            CurKit = MakeHolder<TWordformKit>(Lemmas[Index], 0);
        }
    }

    bool IsValid() const override {
        return Index < Lemmas.size();
    }

    size_t FormsCount() const override {
        return Lemmas.size();
    }
};

class TAutomorphologyFormGenerator: public NLemmer::TFormGenerator {
public:
    TAutomorphologyFormGenerator(const TParadigm& forms, const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter = nullptr)
        : TFormGenerator(lemma, filter)
    {
        Filter->SetLemma(lemma);
        ItPtr.Reset(new TAutomorphologyFormIterator(forms));
    }
};

TAutoPtr<NLemmer::TFormGenerator> TAutomorphologyLanguage::Generator(const TYandexLemma& lemma, const NLemmer::TGrammarFiltr* filter) const {
    Y_ASSERT(lemma.GetLanguageVersion() == Version);
    if (lemma.GetQuality() & TYandexLemma::QFoundling) {
        return new TAutomorphologyFormGenerator(NAutomorphology::SingleWordParadigm(lemma.GetText()), lemma, filter);
    }
    return new TAutomorphologyFormGenerator(SimpleLister.GetForms(lemma.GetText()), lemma, filter);
}

TAutoPtr<NLemmer::TGrammarFiltr> TAutomorphologyLanguage::GetGrammFiltr(const char* /*gram*/) const {
    return new NLemmer::NDetail::TDummyGrammarFiltr();
}

THashMap<ELanguage, TSimpleSharedPtr<TAutomorphologyLanguage>> TAutomorphologyLanguage::LoadedLangs;

#include "simple_simhash.h"
#include <kernel/idf/idf.h>
#include <library/cpp/html/entity/htmlentity.h>
#include <kernel/lemmer/core/language.h>
#include <util/charset/wide.h>
#include <util/digest/city.h>
#include <util/digest/fnv.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/hash.h>
#include <util/generic/noncopyable.h>
#include <util/generic/string.h>
#include <util/generic/ptr.h>
#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/stream/output.h>

const double STOP_WORD_RELATIVE_FREQ = 4.7e-4;

TSimpleSimhashHandler::TSimpleSimhashHandler() {
    FreqCache.Init(21, 1 << 20);
}

void TSimpleSimhashHandler::Reset(TSimpleSharedPtr<TPureContainer> pureContainer, const TLangMask& langMask) {
    Y_ENSURE(pureContainer != nullptr, "pureContainer == nullptr");
    Y_ENSURE(!langMask.Empty(), "langMask is empty");

    PureContainer = std::move(pureContainer);
    LangMask = langMask;

    TotalCollectionLength = 0;
    for (auto collection: LangMask) {
        TotalCollectionLength += PureContainer ->Get(collection).GetCollectionLength();
    }
    Y_ENSURE(TotalCollectionLength != 0, "TotalCollectionLength = 0");
    Doc++;
    Freqs.clear();
}

void TSimpleSimhashHandler::OnTokenStart(const TWideToken& tok, const TNumerStat& /*stat*/) {
    if (DetectNLPType(tok.SubTokens) == NLP_WORD) {
        for (size_t i = 0; i < tok.SubTokens.size(); i++)
            ProcessWord(tok.Token + tok.SubTokens[i].Pos, tok.SubTokens[i].Len);
    }
}




// Most stop words from the lists have relative frequency greater than this value

ui64 TSimpleSimhashHandler::GetSimhash() const {
    Y_ENSURE(PureContainer != nullptr && !LangMask.Empty(),
            "TSimpleSimhashHandler is not initialized, call Reset() first");

    double values[BIT_COUNT];
    Fill(&values[0], &values[BIT_COUNT], 0);
    for (auto& f: Freqs) {
        double weight = f.Idf * f.Count;
        ui64 lh = f.LemmaHash;
        for (size_t i = 0; i < BIT_COUNT; i++, lh >>= 1) {
            values[i] += weight * (i64)(((lh & 1) << 1) - 1);
        }
    }

    ui64 result = 0;
    for (size_t i = 0; i < BIT_COUNT; i++) {
        if (values[i] >= 0)
            result |= 1LLU << i;
    }
    return result;
}

void TSimpleSimhashHandler::ProcessWord(const wchar16* w, ui32 len) {
    wchar16* word = static_cast<wchar16*>(alloca(len * sizeof(wchar16)));
    for (ui32 i = 0; i < len; i++) {
        word[i] = ToLower(w[i]);
    }
    const wchar16* cword = word;
    ui64 hash = (FnvHash<ui64>(word, len * sizeof(wchar16)) * FNV64PRIME) ^ LangMask.GetHash();
    TFreqsDoc* freqs = FreqCache.FindOrCalc(hash, [this, &len, &cword](ui64, TFreqsDoc& freqs) {
        TWLemmaArray lemmas;
        NLemmer::AnalyzeWord(cword, len, lemmas, LangMask);
        if (!lemmas.empty()) {
            auto& lemma = lemmas[0];
            cword = lemma.GetText();
            len = lemma.GetTextLength();
        }
        freqs.LemmaHash = CityHash64(reinterpret_cast<const char*>(cword), len * sizeof(wchar16));
        ui64 freq = 0;
        for (auto it = LangMask.begin(); it != LangMask.end(); ++it) {
            freq += PureContainer->Get(*it).GetByLex(cword, len, NPure::ECase::AllCase, *it).GetFreq();
        }
        double relativeFrequency = (freq + 0.1) / TotalCollectionLength;
        if (relativeFrequency > STOP_WORD_RELATIVE_FREQ) {
            freqs.Idf = 0;
        }
        else {
            freqs.Idf = ReverseFreq2Idf(1.0 / relativeFrequency);
            freqs.Index = Freqs.size();
            freqs.Doc = Doc;
            Freqs.push_back(TFreqs(freqs.LemmaHash, freqs.Idf));
        }
    });
    if (freqs->Idf != 0) {
        if (freqs->Doc != Doc) {
            freqs->Doc = Doc;
            freqs->Index = Freqs.size();
            Freqs.push_back(TFreqs(freqs->LemmaHash, freqs->Idf));
        }
        Freqs[freqs->Index].Count++;
    }
}

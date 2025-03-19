#include "url_analyzer.h"

#include <kernel/url_text_analyzer/lib/analyze.h>
#include <kernel/url_text_analyzer/lib/impl.h>

#include <kernel/qtree/richrequest/wordnode.h>
#include <kernel/qtree/richrequest/loadfreq.h>

#include <ysite/yandex/pure/pure.h>
#include <kernel/lemmer/untranslit/untranslit.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/core/language.h>
#include <util/string/split.h>
#include <util/generic/ylimits.h>
#include <algorithm>

#include <library/cpp/cache/cache.h>
#include <util/system/spinlock.h>
#include <util/digest/murmur.h>
#include <util/string/cast.h>


namespace NUta::NPrivate {
    struct TWordData {
        double Probability = 0.0;
        bool IsBastard = true;

        TWordData() = default;
        TWordData(double wordProbability, bool isBastard)
            : Probability(wordProbability)
            , IsBastard(isBastard)
        {
        }
    };

    class TSmartUrlAnalyzerImpl: public IUrlAnalyzerImpl {
    public:
        TSmartUrlAnalyzerImpl(const TVector<TString>& pureFiles, size_t cacheSize = 0, size_t bucketNum = 0);

        TSmartUrlAnalyzerImpl() = default;

        bool TryUntranslit(const TUtf16String& word, TUtf16String* result, ui32 maxTranslitCandidates) const override;

        TVector<TUtf16String> FindSplit(const TUtf16String& word,
            bool penalizeForLength,
            bool tryUntranslit,
            ui32 minSubTokenLen,
            ui32 maxTranslitCandidates) const override;

    private:
        void GetRevFr(TWordNode& wordNode, double* wordRevFr) const;
        TWordData GetWordData(const TUtf16String& word, bool penalizeForLength) const;

    private:

        struct TStatData {
            TVector<TPure> Pures;

            TStatData(const size_t nPures)
                : Pures(nPures)
            {
            }
        };

    private:
        THolder<TStatData> StatData;
        TVector<THolder<TLRUCache<TUtf16String, TUtf16String>>> UtrCache;
        TVector<THolder<TLRUCache<TUtf16String, TWordData>>> LmrCache;
        TVector<TAdaptiveLock> UtrLock;
        TVector<TAdaptiveLock> LmrLock;
        bool UseCache = false;
        size_t BucketNum = 0;
        static const TVector<ELanguage> SupportedLangs;
    };

    TSmartUrlAnalyzerImpl::TSmartUrlAnalyzerImpl(const TVector<TString>& pureFiles, size_t cacheSize, size_t bucketNum)
        : StatData(new TStatData(pureFiles.size()))
    {
        if ((cacheSize != 0) && (bucketNum != 0)) {
            UseCache = true;
            BucketNum = bucketNum;
            UtrLock.resize(bucketNum);
            LmrLock.resize(bucketNum);
            LmrCache.resize(bucketNum);
            UtrCache.resize(bucketNum);
            for (size_t i = 0; i < bucketNum; ++i) {
                UtrCache[i] = MakeHolder<TLRUCache<TUtf16String, TUtf16String>>(cacheSize);
                LmrCache[i] = MakeHolder<TLRUCache<TUtf16String, TWordData>>(cacheSize);
            }
        }
        for (size_t i = 0; i < pureFiles.size(); ++i) {
            StatData->Pures[i].Init(pureFiles[i]);
        }
    }

    const TVector<ELanguage> TSmartUrlAnalyzerImpl::SupportedLangs = {LANG_RUS};

    bool TSmartUrlAnalyzerImpl::TryUntranslit(const TUtf16String& word, TUtf16String* result, ui32 maxTranslitCandidates) const {
        size_t cacheBucketId = 0;
        if (UseCache) {
            cacheBucketId = MurmurHash<size_t>((const char*) word.data(), word.size() * sizeof(wchar16)) % BucketNum;
            TGuard<TAdaptiveLock> guard(UtrLock[cacheBucketId]);
            auto& cache = UtrCache[cacheBucketId];
            const auto& it = cache->Find(word);
            if (it != cache->End()) {
                if (it.Value().empty()) {
                    return false;
                }
                *result = it.Value();
                return true;
            }
        }
        TAutoPtr<TWordNode> wordInfo(TWordNode::CreateLemmerNode(word, TCharSpan(0, word.size()), fGeneral, TLanguageContext(LI_BASIC_LANGUAGES), false));
        double wordRevFr = 0.0;
        GetRevFr(*wordInfo, &wordRevFr);
        bool resultOk = false;

        for (const auto lang : SupportedLangs) {
            auto untransliter =
                NLemmer::GetLanguageById(lang)->GetUntransliter(word);
            if (untransliter == nullptr) {
                continue;
            }

            ui32 variantsLeft = maxTranslitCandidates;
            while (variantsLeft-- != 0) {
                TUntransliter::WordPart wordPart = untransliter->GetNextAnswer();
                const auto& answer = wordPart.GetWord();
                if (answer.empty() || answer == word || answer == *result) {
                    break;
                }
                TAutoPtr<TWordNode> ansInfo(TWordNode::CreateLemmerNode(answer, TCharSpan(0, answer.size()), fGeneral, TLanguageContext(LI_BASIC_LANGUAGES), false));
                double ansRevFr = 0.0;
                GetRevFr(*ansInfo, &ansRevFr);
                if (ansRevFr == 0.0) {
                    continue;
                }

                if (wordRevFr == 0.0 || ansRevFr < wordRevFr) {
                    wordRevFr = ansRevFr;
                    *result = answer;
                    resultOk = true;
                }

            }
        }
        if (UseCache) {
            TGuard<TAdaptiveLock> guard(UtrLock[cacheBucketId]);
            auto& cache = UtrCache[cacheBucketId];
            if (Y_LIKELY(!resultOk)) {
                cache->Insert(word, TUtf16String());
            } else {
                cache->Insert(word, *result);
            }
        }
        return resultOk;
    }

    TVector<TUtf16String> TSmartUrlAnalyzerImpl::FindSplit(const TUtf16String& word,
            bool penalizeForLength,
            bool tryUntranslit,
            ui32 minSubTokenLen,
            ui32 maxTranslitCandidates) const {
        const int length = word.size();
        TVector<double> probabilities(length + 1, -Max<double>());
        TVector<size_t> offsets(length + 1, length + 1);
        probabilities[length] = 0.0;
        TVector<TUtf16String> splittedWords(length);
        TVector<TWordData>    wordDatas(length);

        for (int i = length - 1; i >= 0; --i) {
            for (int j = i; j < length; ++j) {
                if (ui32(j - i + 1) < minSubTokenLen || probabilities[j + 1] == -Max<double>()) {
                    continue;
                }
                TUtf16String part = word.substr(i, j - i + 1);

                TUtf16String untraslited;
                if (tryUntranslit && TryUntranslit(part, &untraslited, maxTranslitCandidates)) {
                    std::swap(part, untraslited);
                }

                const auto wd = GetWordData(part, penalizeForLength);
                auto newProb = wd.Probability;
                if (newProb > -Max<double>()) {
                    newProb += probabilities[j + 1];
                    if (newProb > probabilities[i]) {
                        probabilities[i] = newProb;
                        std::swap(splittedWords[i], part);
                        wordDatas[i] = wd;
                        offsets[i] = j - i + 1;
                    }
                }
            }
        }

        double wordProb = GetWordData(word, penalizeForLength).Probability;
        if (Y_LIKELY(wordProb > probabilities[0])) {
            return {word};
        } else if (probabilities[0] > -Max<double>()) {
            TVector<TUtf16String> split;
            split.reserve(splittedWords.size());

            const auto firstWordData = wordDatas[0];
            split.push_back(std::move(splittedWords[0]));

            bool previousIsBastard = firstWordData.IsBastard;
            for (int i = offsets[0]; i < length; i += offsets[i]) {
                bool currentIsBastard = wordDatas[i].IsBastard;
                if (previousIsBastard && currentIsBastard) {
                    split.back() += splittedWords[i];
                } else {
                    split.push_back(std::move(splittedWords[i]));
                    previousIsBastard = currentIsBastard;
                }
            }
            return split;
        } else {
            return {};
        }
    }

    void TSmartUrlAnalyzerImpl::GetRevFr(TWordNode& wordNode, double* wordRevFr) const {
        *wordRevFr = Max<double>();
        for (const auto& pure : StatData->Pures) {
            LoadFreq(pure, wordNode);
            const ui64 curFr = static_cast<ui64>(wordNode.GetRevFr());
            if (curFr < pure.GetCollectionLength()) {
                *wordRevFr = ::Min(*wordRevFr, double(curFr));
            }
        }
        *wordRevFr *= double(*wordRevFr != Max<double>());
    }

    TWordData TSmartUrlAnalyzerImpl::GetWordData(const TUtf16String& word, bool penalizeForLength) const {
        size_t cacheBucketId = 0;
        if (UseCache) {
            cacheBucketId = MurmurHash<size_t>((const char*) word.data(), word.size() * sizeof(wchar16)) % BucketNum;
            TGuard<TAdaptiveLock> guard(LmrLock[cacheBucketId]);
            auto& cache = LmrCache[cacheBucketId];
            const auto& it = cache->Find(word);
            if (it != cache->End()) {
                return it.Value();
            }
        }
        double probability = -Max<double>();
        TAutoPtr<TWordNode> wordInfo(TWordNode::CreateLemmerNode(word, TCharSpan(0, word.size()), fGeneral, TLanguageContext(LI_BASIC_LANGUAGES), false));
        double wordRevFr = 0.0;
        GetRevFr(*wordInfo, &wordRevFr);
        if (wordRevFr > 0.0) {
            probability = -log(wordRevFr);
            if (penalizeForLength) {
                probability *= (word.size() < 3) ? 1.0
                    : 1.0 / (static_cast<double>(word.size()) - 2.0);
            }
        }
        const auto result = TWordData{probability, wordInfo->IsBastard()};
        if (UseCache) {
            TGuard<TAdaptiveLock> guard(LmrLock[cacheBucketId]);
            auto& cache = LmrCache[cacheBucketId];
            cache->Insert(word, result);
        }
        return result;
    }
}

namespace NUta {
    using namespace NPrivate;

    TSmartUrlAnalyzer::TSmartUrlAnalyzer(const TVector<TString>& pureFiles, const TOpts& options)
            : Options(options) {
        Impl.Reset(new TSmartUrlAnalyzerImpl(pureFiles, options.CacheSize, options.BucketNum));
    }

    TSmartUrlAnalyzer::TSmartUrlAnalyzer(const TFastStrictOpts& fastOptions)
            : Options(fastOptions) {
        Impl.Reset(new TSmartUrlAnalyzerImpl());
    }

    TSmartUrlAnalyzer::TSmartUrlAnalyzer(const TLightExperimentalOpts& expOpts)
            : Options(expOpts) {
        Impl.Reset(new TSmartUrlAnalyzerImpl());
    }

    TSmartUrlAnalyzer::~TSmartUrlAnalyzer() = default;

    TVector<TString> TSmartUrlAnalyzer::AnalyzeUrlUTF8(const TStringBuf& url) const {
        return AnalyzeUrlUTF8Impl(Impl, Options, url);
    }

} // NUta

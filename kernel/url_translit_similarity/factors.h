#pragma once
#include "translate.h"
#include <util/string/split.h>

namespace NUrlTranslitSimilarity {
    struct TFeature {
        enum EId {
            DomainCovered,
            PathCovered,
            QueryCovered,
            AllCovered,
            AvgPathSensePerDomain,
            DomainSize,
            Count
        };
    };

    // для последовательности слов добавляю двух-трех-словные фразы
    TVector<TString> MakeWordNGrams(const TVector<TString>& words);
    TVector<TString> MakeWordNGrams(const TVector<TString>& words, int maxLength);

    class TDomainIdfSize {
    public:
        TMap<TString, double> DomainToMeaningfulWeight;
        double SumDomainSize = 0.0;
        TMap<TString, double> DomainToSize;

        int Load(IInputStream* inputStream);

        double GetDomainIdfFactor(const TString& domainStr) const {
            double avgDomainIdf = 0.5;

            TMap<TString, double>::const_iterator tw = DomainToMeaningfulWeight.find(domainStr);
            if (tw != DomainToMeaningfulWeight.end()) {
                avgDomainIdf = tw->second;
            }
            return (1.0 - avgDomainIdf);
        }

        double GetDomainSizeFactor(const TString& domainStr) const {
            Y_ENSURE(SumDomainSize > 0.5);
            double domainSize = 1.0;

            TMap<TString, double>::const_iterator tw = DomainToSize.find(domainStr);
            if (tw != DomainToSize.end()) {
                domainSize = tw->second;
            }
            return domainSize / SumDomainSize;
        }
    };

    class TTranslitWords {
    public:
        void Clear(const TString& word) {
            Score = 0.0;
            Translit.clear();
            Word = word;
        }

        bool IsHit() const {
            if (Translit.empty()) {
                return false;
            }
            double dLength = Max(1.0, double(PosUtf8Length(Word)));
            double relScore = Score / dLength;
            const double longWeightThreshold = 3.0;
            const double shortWeightThreshold = 1.2;
            bool uTokenIsHit = (relScore < longWeightThreshold);
            if ((dLength < 3.5) && (relScore > shortWeightThreshold)) {
                uTokenIsHit  = false;
            }
            return uTokenIsHit;
        }

        TString Word;
        TVector<TString> Translit;
        double Score = 0.0;
    };

    class TWordsFactor {
    public:
        TMap<TString, double> WordIdf;
        TMap<TString, double> DomainIdf;
        TMap<TString, double> PathIdf;

        double DomainSumFreq = 0.0;
        double PathSumFreq = 0.0;

        int LoadDomainPath(IInputStream* inputStream);
        int LoadWordFreqs(IInputStream* inputStream);

        TVector<float> GetFactors(
            const TVector<TTranslitWords>& tokensP,
            const TVector<TTranslitWords>& tokensD,
            const TVector<TString>& qwords) const;
    };

    struct TFactorsData {
        IInputStream* LettersPtStream = nullptr;
        IInputStream* WordsPtStream = nullptr;
        IInputStream* UrlTokensFreqsStream = nullptr;
        IInputStream* WordFreqsStream = nullptr;
        IInputStream* DomainIdfStream = nullptr;
    };

    class TTranslitIdfFactors {
    public:
        void Load(const TFactorsData& cfg, double maxWeight, size_t maxWidth) {
            DomainIdfSize.Load(cfg.DomainIdfStream);
            WordsFactor.LoadWordFreqs(cfg.WordFreqsStream);
            WordsFactor.LoadDomainPath(cfg.UrlTokensFreqsStream);
            LettersDecode.LoadPhraseModel(cfg.LettersPtStream, true, maxWeight, maxWidth);
            VocabTransit.Load(cfg.WordsPtStream);
        }

        // то же что и Load, но с отладочной печатью
        void LoadDebug(const TFactorsData& cfg, double maxWeight, size_t maxWidth) {
            Cerr << " DomainIdfSize " << DomainIdfSize.Load(cfg.DomainIdfStream);
            Cerr << Endl;
            Cerr << " LoadWordFreqs " << WordsFactor.LoadWordFreqs(cfg.WordFreqsStream);
            Cerr << Endl;
            Cerr << " LoadDomainPath " << WordsFactor.LoadDomainPath(cfg.UrlTokensFreqsStream);
            Cerr << Endl;
            Cerr << " LoadPhraseModel " << LettersDecode.LoadPhraseModel(cfg.LettersPtStream,
                true, maxWeight, maxWidth);
            Cerr << Endl;
            Cerr << " VocabTransit " << VocabTransit.Load(cfg.WordsPtStream);
            Cerr << Endl;
        }

        TVector<float> ProcessStr(
            const TString& queryStr,
            const TVector<TString>& domainWords,
            const TVector<TString>& pathWords,
            size_t wordsNumberLimit = 10) const
        {
            TString domainStr = JoinSeq(" ", domainWords);
            TVector<TString> queryWords;
            StringSplitter(queryStr).Split(' ').AddTo(&queryWords);
            if (queryWords.size() > wordsNumberLimit) {
                queryWords.resize(wordsNumberLimit);
            }

            int maxLength1 = GetMaxUtf8LengthBound(pathWords);
            int maxLength2 = GetMaxUtf8LengthBound(domainWords);

            TVector<TString> wordNGrams = MakeWordNGrams(queryWords, 4 + Max(maxLength1, maxLength2));
            Sort(wordNGrams);

            TVector<TTranslitWords> pathTokens = TranslitText(pathWords, wordNGrams);
            TVector<TTranslitWords> domainTokens = TranslitText(domainWords, wordNGrams);

            TVector<float> factors = WordsFactor.GetFactors(pathTokens, domainTokens, queryWords);
            factors[TFeature::AvgPathSensePerDomain] = static_cast<float>(DomainIdfSize.GetDomainIdfFactor(domainStr));
            factors[TFeature::DomainSize] = static_cast<float>(DomainIdfSize.GetDomainSizeFactor(domainStr));
            return factors;
        }

        TVector<TTranslitWords> TranslitText(
            const TVector<TString>& keyForms,
            const TVector<TString>& wordNGrams) const
        {
            TVector<TTranslitWords> tokens;
            tokens.resize(keyForms.size());
            for (size_t wi = 0; wi < tokens.size(); wi++) {
                tokens[wi].Clear(keyForms[wi]);
            }
            if (wordNGrams.empty()) {
                return tokens;
            }
            for (auto& token : tokens) {
                if (token.Word.empty()) {
                    continue;
                }
                TPtRec translationResult;
                if (VocabTransit.GetBestTranslation(token.Word, wordNGrams, translationResult)) {
                    token.Translit.push_back(translationResult.Dst);
                    continue;
                }
                TDecodeResult decodeResult = LettersDecode.SearchWords(token.Word, wordNGrams);
                if (decodeResult.WordNum >= 0) {
                    StringSplitter(wordNGrams[decodeResult.WordNum]).Split('~').AddTo(&token.Translit);
                    token.Score = decodeResult.Weight;
                }
            }
            return tokens;
        }
    private:
        TDomainIdfSize DomainIdfSize;
        TWordsFactor WordsFactor;
        TLettersDecode LettersDecode;
        TVocabTransit VocabTransit;
    };
}

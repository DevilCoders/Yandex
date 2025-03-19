#include "factors.h"
#include <util/string/split.h>

namespace NUrlTranslitSimilarity {
    // Чтение файла DomainavIdf
    int TDomainIdfSize::Load(IInputStream* inputStream) {
        SumDomainSize = 0.0;
        DomainToMeaningfulWeight.clear();
        DomainToSize.clear();

        int line = 0;
        TString entry;

        while (inputStream->ReadLine(entry)) {
            line++;
            TrimEolSpace(entry);
            TVector<TString> parts;
            StringSplitter(entry).Split('\t').AddTo(&parts);
            Y_ENSURE(parts.size() == 3);

            double domainSize = FromString<double>(parts[2]);
            DomainToMeaningfulWeight[parts[0]] = FromString<double>(parts[1]);
            DomainToSize[parts[0]] = domainSize;
            SumDomainSize += domainSize;
        }
        return line;
    }

    // Чтение файла WordFreqs
    int TWordsFactor::LoadWordFreqs(IInputStream* inputStream) {
        double wordSumFreq = 0.0;
        WordIdf.clear();

        int line = 0;
        TString entry;

        while (inputStream->ReadLine(entry)) {
            line++;
            TrimEolSpace(entry);
            TVector<TString> parts;
            StringSplitter(entry).Split('\t').AddTo(&parts);
            Y_ENSURE(parts.size() == 2);

            double freq = FromString<double>(parts[1]);
            WordIdf[parts[0]] = freq;
            wordSumFreq += freq;
            Y_ENSURE(freq > 0.5);
        }
        Y_ENSURE(wordSumFreq > 0.5);

        for (auto& tword : WordIdf) {
            tword.second = log(wordSumFreq / tword.second);
        }
        return line;
    }

    // Чтение файла DomainPath
    int TWordsFactor::LoadDomainPath(IInputStream* inputStream) {
        DomainSumFreq = 0.0;
        PathSumFreq = 0.0;

        DomainIdf.clear();
        PathIdf.clear();

        int line = 0;
        TString entry;

        while (inputStream->ReadLine(entry)) {
            line++;
            TrimEolSpace(entry);
            TVector<TString> parts;
            StringSplitter(entry).Split('\t').AddTo(&parts);
            // token, type, freq
            Y_ENSURE(parts.size() == 3);

            double freq = FromString<double>(parts[2]);
            Y_ENSURE(freq > 0.5);
            if (parts[1] == "domain") {
                DomainIdf[parts[0]] = freq;
                DomainSumFreq += freq;
            }
            else {
                Y_ENSURE(parts[1] == "path");
                PathIdf[parts[0]] = freq;
                PathSumFreq += freq;
            }
        }
        Y_ENSURE(DomainSumFreq > 0.5);
        Y_ENSURE(PathSumFreq > 0.5);
        for (auto& tword : DomainIdf) {
            tword.second = log(DomainSumFreq / tword.second);
        }
        for (auto& tword : PathIdf) {
            tword.second = log(PathSumFreq / tword.second);
        }
        return line;
    }

    // Вычисление четырёх факторов, которые зависят от запроса и урла
    TVector<float> TWordsFactor::GetFactors(
        const TVector<TTranslitWords>& tokensP,
        const TVector<TTranslitWords>& tokensD,
        const TVector<TString>& qwords) const
    {
        double sumDomainPositive = 0.0;
        double sumPathPositive = 0.0;
        double maxDomain = 0.0;
        double maxPath = 0.0;

        TSet<TString> hitWordsQ;
        TSet<TString> wordsQ;
        wordsQ.insert(qwords.begin(), qwords.end());
        TMap<TString, TSet<TString>> bigramsQ;
        for (size_t wi = 0; wi < qwords.size(); wi++) {
            if (wi > 0) {
                const TString cs = qwords[wi - 1] + qwords[wi];
                bigramsQ[cs].insert(qwords[wi - 1]);
                bigramsQ[cs].insert(qwords[wi]);
            }
        }

        for (const auto& t : tokensD) {
            double weight = 10.0;
            TMap<TString, double>::const_iterator it = DomainIdf.find(t.Word);
            if (it != DomainIdf.end()) {
                weight = it->second;
            }
            bool isInQuery = ((wordsQ.find(t.Word) != wordsQ.end()) ||
                (bigramsQ.find(t.Word) != bigramsQ.end()));
            maxDomain += weight;
            if (isInQuery) {
                hitWordsQ.insert(t.Word);
                sumDomainPositive += weight;
            }
            else {
                if (t.IsHit())  {
                    hitWordsQ.insert(t.Translit.begin(), t.Translit.end());
                    sumDomainPositive += weight;
                }
            }
        }

        for (const auto& t : tokensP) {
            double weight = 10.0;
            TMap<TString, double>::const_iterator it = PathIdf.find(t.Word);
            if (it != PathIdf.end()) {
                weight = it->second;
            }

            bool isInQuery = (wordsQ.find(t.Word) != wordsQ.end());
            if (bigramsQ.find(t.Word) != bigramsQ.end()) {
                isInQuery = true;
            }
            maxPath += weight;
            if (isInQuery) {
                hitWordsQ.insert(t.Word);
                sumPathPositive += weight;
            }
            else {
                if (t.IsHit())  {
                    hitWordsQ.insert(t.Translit.begin(), t.Translit.end());
                    sumPathPositive += weight;
                }
            }
        }

        for (const auto& tb : bigramsQ) {
            if (hitWordsQ.find(tb.first) != hitWordsQ.end()) {
                hitWordsQ.insert(tb.second.begin(), tb.second.end());
            }
        }

        double maxQuery = 0.0;
        double sumQuery = 0.0;
        for (const auto& tword : wordsQ) {
            double weight = 15.0;
            TMap<TString, double>::const_iterator x = WordIdf.find(tword);
            if (x != WordIdf.end()) {
                weight = x->second;
            }
            maxQuery += weight;
            if (hitWordsQ.find(tword) != hitWordsQ.end()) {
                sumQuery += weight;
            }
        }

        TVector<float> factors(TFeature::Count, 0.0f);
        const double smallValue = 1e-6;

        if (maxDomain > smallValue) {
            factors[TFeature::DomainCovered] = static_cast<float>(sumDomainPositive / maxDomain);
        }
        if (maxPath > smallValue) {
            factors[TFeature::PathCovered] = static_cast<float>(sumPathPositive / maxPath);
        }
        if (maxQuery > smallValue) {
            factors[TFeature::QueryCovered] = static_cast<float>(sumQuery / maxQuery);
        }
        double sum3 = maxDomain + maxPath + maxQuery;
        if (sum3 > smallValue) {
            factors[TFeature::AllCovered] = static_cast<float>((sumDomainPositive + sumPathPositive + sumQuery) / sum3);
        }
        return factors;
    }

    TVector<TString> MakeWordNGrams(const TVector<TString>& words) {
        TVector<TString> wordNGrams = words;

        TString divStr = "~";
        for (size_t i = 1; i < words.size(); i++) {
            wordNGrams.push_back(words[i - 1] + divStr + words[i]);
            wordNGrams.push_back(words[i] + divStr + words[i - 1]);
        }
        for (size_t i = 2; i < words.size(); i++) {
            wordNGrams.push_back(words[i - 2] + divStr + words[i - 1] + divStr + words[i]);
        }
        return wordNGrams;
    }

    TVector<TString> MakeWordNGrams(const TVector<TString>& words, int maxLength) {
        TVector<TString> wordNGrams;

        TVector<int> dstlen;
        dstlen.resize(words.size());
        for (size_t i = 0; i < words.size(); i++) {
            dstlen[i] = PosUtf8Length(words[i]);
        }

        for (size_t i = 0; i < words.size(); i++) {
            if (dstlen[i] <= maxLength) {
                wordNGrams.push_back(words[i]);
            }
        }
        TString divStr = "~";
        for (size_t i = 1; i < words.size(); i++) {
            if (dstlen[i - 1] + dstlen[i] < maxLength) {
                wordNGrams.push_back(words[i - 1] + divStr + words[i]);
                wordNGrams.push_back(words[i] + divStr + words[i - 1]);
            }
        }
        for (size_t i = 2; i < words.size(); i++) {
            if (dstlen[i - 2] + dstlen[i - 1] + dstlen[i] < maxLength) {
                wordNGrams.push_back(words[i - 2] + divStr + words[i - 1] + divStr + words[i]);
            }
        }
        return wordNGrams;
    }
}

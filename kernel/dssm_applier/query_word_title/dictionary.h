#pragma once

#include <util/memory/blob.h>
#include <util/generic/hash.h>
#include <util/generic/vector.h>

namespace NQueryWordTitle {

struct TQueryWordDictionary {
public:
    static const size_t INVALID_WORD_INDEX = Max<size_t>();

    TQueryWordDictionary(const TString& wordsFile, const TString& vectorsFile);
    TQueryWordDictionary(IInputStream& wordsStream, IInputStream& vectorsStream);
    static void SaveToFile(const TString& outWordsFileName, const TString& outVectorsFileName, const TVector<TString>& words, const TVector<TVector<float>>& vectors,
                    size_t vectorSize, float multiplier, float bias, size_t lowSupportedVersion, size_t highSupportedVersion);

    //to find word embed use GetWordIndex and GetVectorByIndex next
    //GetWordIndex returns INVALID_WORD_INDEX, if word is not in dictionary
    size_t GetWordIndex(const TString& word) const;
    //note: result vector is always normalized
    TVector<float> GetVectorByIndex(size_t index) const;

    size_t Size() const {
        return WordToIndex.size();
    }

    size_t GetVectorSize() const {
        return VectorSize;
    }

    float GetMultiplier() const {
        return Multiplier;
    }

    float GetBias() const {
        return Bias;
    }

    size_t GetLowSupportedVersion() const {
        return LowSupportedVersion;
    }

    size_t GetHighSupportedVersion() const {
        return HighSupportedVersion;
    }

    bool IsSupportedVersion(size_t version) const {
        return version >= LowSupportedVersion && version <= HighSupportedVersion;
    }

private:
    void ReadWordsPart(IInputStream& stream);

    THashMap<TString, size_t> WordToIndex;
    TBlob Vectors;
    size_t VectorSize = 0;
    float Multiplier = 0;
    float Bias = 0;
    size_t LowSupportedVersion = 0;
    size_t HighSupportedVersion = 0;
};

};

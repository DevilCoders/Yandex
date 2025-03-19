#include "dictionary.h"

#include <util/stream/file.h>

#include <kernel/dssm_applier/utils/utils.h>
#include <kernel/dssm_applier/begemot/production_data.h>

NQueryWordTitle::TQueryWordDictionary::TQueryWordDictionary(const TString& wordsFile, const TString& vectorsFile) {
    {
        TFileInput fileInput(wordsFile);
        ReadWordsPart(fileInput);
    }
    {
        Vectors = TBlob::PrechargedFromFile(vectorsFile);
        Y_ENSURE(Vectors.Size() == Size() * VectorSize * sizeof(ui8));
    }
}

NQueryWordTitle::TQueryWordDictionary::TQueryWordDictionary(IInputStream& wordsStream, IInputStream& vectorsStream) {
    ReadWordsPart(wordsStream);
    Vectors = TBlob::FromStream(vectorsStream);
    Y_ENSURE(Vectors.Size() == Size() * VectorSize * sizeof(ui8));
}

void NQueryWordTitle::TQueryWordDictionary::ReadWordsPart(IInputStream& stream) {
    size_t count = 0;
    stream >> count >> VectorSize >> Multiplier >> Bias >> LowSupportedVersion >> HighSupportedVersion;
    for (size_t i = 0; i < count; ++i) {
        TString word;
        stream >> word;
        WordToIndex[word] = i;
    }
}

void NQueryWordTitle::TQueryWordDictionary::SaveToFile(const TString& outWordsFileName, const TString& outVectorsFileName, const TVector<TString>& words, const TVector<TVector<float>>& vectors,
                    size_t vectorSize, float multiplier, float bias, size_t lowSupportedVersion, size_t highSupportedVersion) {
    {
        TFixedBufferFileOutput outputStream(outWordsFileName);
        outputStream << words.size() << " " << vectorSize << " "
            << multiplier << " " << bias << " " << lowSupportedVersion << " " << highSupportedVersion << Endl;
        for (const TString& word : words) {
            outputStream << word << Endl;
        }
    }
    {
        TFixedBufferFileOutput outputStream(outVectorsFileName);
        for (const auto& vect : vectors) {
            Y_ENSURE(vect.size() == vectorSize);
            TVector<ui8> compressedVect = NDssmApplier::NUtils::TFloat2UI8Compressor::Compress(vect);
            outputStream.Write(compressedVect.data(), vectorSize * sizeof(ui8));
        }
    }
}

size_t NQueryWordTitle::TQueryWordDictionary::GetWordIndex(const TString& word) const {
    auto it = WordToIndex.find(word);
    return (it == WordToIndex.end()) ? INVALID_WORD_INDEX : it->second;
}

TVector<float> NQueryWordTitle::TQueryWordDictionary::GetVectorByIndex(size_t index) const {
    Y_ENSURE(index < Size());
    const ui8* begin = static_cast<const ui8*>(Vectors.Data()) + index * VectorSize;
    TVector<float> result = NDssmApplier::NUtils::TFloat2UI8Compressor::Decompress(TArrayRef<const ui8>(begin, VectorSize));
    bool normSuccess = NNeuralNetApplier::TryNormalize(result);
    Y_ENSURE(normSuccess);
    return result;
}

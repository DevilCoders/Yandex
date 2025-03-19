#pragma once

#include "tokenizer.h"

namespace NNeuralNetApplier {

class TTokenizerBuilder {
private:
    TTermToIndex Mapping;
    TString TokenizerTypeName;
    bool LowercaseInput = false;
    bool UseUnknownWord = false;
    size_t UnknownWordId = 0;
    size_t Version = 0;

    size_t CurIndex = 0;

public:
    TTokenizerBuilder() = default;

    TTokenizerBuilder(const TString& tokenizerTypeName, bool lowercaseInput = false,
        bool useUnknownWord = false, size_t unknownWordId = 0, size_t version = 0)
            : TokenizerTypeName(tokenizerTypeName)
            , LowercaseInput(lowercaseInput)
            , UseUnknownWord(useUnknownWord)
            , UnknownWordId(unknownWordId)
            , Version(version)
    {}

    TAtomicSharedPtr<ITokenizer> GetTokenizer() const {
        if (TokenizerTypeName == "TTrigramTokenizer") {
            return new TTrigramTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId);
        } else if (TokenizerTypeName == "TWordTokenizer") {
            return new TWordTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId, Version);
        } else if (TokenizerTypeName == "TPhraseTokenizer") {
            return new TPhraseTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId);
        } else if (TokenizerTypeName == "TBigramsTokenizer") {
            return new TBigramsTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId, Version);
        } else if (TokenizerTypeName == "TCachedTrigramTokenizer") {
            return new TCachedTrigramTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId);
        } else if (TokenizerTypeName == "TWideBigramsTokenizer") {
            return new TWideBigramsTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId);
        } else if (TokenizerTypeName == "TPrefixTokenizer") {
            return new TPrefixTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId);
        } else if (TokenizerTypeName == "TSuffixTokenizer") {
            return new TSuffixTokenizer(Mapping, LowercaseInput, UseUnknownWord, UnknownWordId);
        }

        ythrow yexception() << "Unknown tokenizer type: " << TokenizerTypeName;
    }

    size_t AddToken(const TUtf16String& key) { // returns new value for entry
        if (CurIndex == UnknownWordId) {
            ++CurIndex;
        }

        if (!Mapping.contains(key)) {
            Mapping[key] = CurIndex;
            ++CurIndex;
        }

        return Mapping.at(key);
    }
};

}

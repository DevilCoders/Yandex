#include <kernel/dssm_applier/nn_applier/lib/tokenizer.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/random/fast.h>
#include <util/random/shuffle.h>
#include <util/stream/str.h>
#include <util/folder/path.h>


using namespace NNeuralNetApplier;

namespace {

NNeuralNetApplier::TTermToIndex ReadResourceDict(TFsPath resourcePath, EStoreStringType storeStringType, ui64 startId, ui64 hashModulo, size_t maxWords) {
    Y_ENSURE(resourcePath.Exists());
    TFileInput fileInput(resourcePath);

    TVector<TString> tokens;
    ::Load(&fileInput, tokens);

    Y_ENSURE(!tokens.empty());

    TTermToIndex dict;
    for (size_t i = 0; i < Min(maxWords, tokens.size()); ++i) {
        TUtf16String wideToken = UTF8ToWide(tokens[i]);
        ui64 hash = TUpdatableDictIndex<ui64>::CalcHash(wideToken, storeStringType);
        if (hashModulo) {
            hash %= hashModulo;
        }
        dict.emplace(wideToken, startId + hash);
    }
    Y_ENSURE(!dict.empty());
    return dict;
}

NNeuralNetApplier::TTermToIndex ReadResourceDict(ETokenType tokenType, bool lowerCase, EStoreStringType storeStringType, ui64 startId, ui64 hashModulo, size_t maxWords) {
    static THashMap<TString, TString> TokenTypeToDict = {
        { "LowerWord", "titles_lw.bin" },
        { "LowerBigram", "titles_lb.bin" },
        { "LowerTrigram", "titles_ll3.bin" },
    };
    TString tokenTypeAlias = (lowerCase ? "Lower" : "") + ToString(tokenType);
    Y_ENSURE(TokenTypeToDict.contains(tokenTypeAlias), "please, create resouce for " << tokenType << " lowercase=" << lowerCase << " for yourself");
    TFsPath tokensFile(TokenTypeToDict.at(tokenTypeAlias));
    return ReadResourceDict(tokensFile, storeStringType, startId, hashModulo, maxWords);
}

TVector<ui64> GetHashes(const TTermToIndex& dict, EStoreStringType storeStringType) {
    TVector<ui64> hashes;
    for (const auto& [token, _] : dict) {
        hashes.push_back(TUpdatableDictIndex<ui64>::CalcHash(token, storeStringType));
    }
    return hashes;
}

TVector<TUtf16String> ReadValidateTexts() {
    TFsPath validateTextsPath = "left_titles_2020_08_07.txt";
    Y_ENSURE(validateTextsPath.Exists());
    TFileInput fileInput(validateTextsPath);

    TVector<TUtf16String> texts;
    for (TString line; fileInput.ReadLine(line); ) {
        texts.push_back(UTF8ToWide(line));
    }
    Y_ENSURE(!texts.empty());
    return texts;
}

}  // unnamed namespace


void ValidateTokenizer(ITokenizer* correctTokenizer, ITokenizer* testTokenizer, const TVector<TUtf16String>& validateTexts, size_t maxLines) {
    for (size_t i = 0; i < validateTexts.size() && i < maxLines; ++i) {
        TUtf16String text = validateTexts[i];
        if (correctTokenizer->NeedLowercaseInput()) {
            text = ToLowerRet(text);
        }
        TVector<size_t> correctIndices;
        TVector<float> correctWeights;
        correctTokenizer->AddTokens(text, correctIndices, correctWeights);

        TVector<size_t> testIndices;
        TVector<float> testWeights;
        testTokenizer->AddTokens(text, testIndices, testWeights);

        UNIT_ASSERT_VALUES_EQUAL_C(correctIndices, testIndices, " different indices for " << text);
        UNIT_ASSERT_VALUES_EQUAL_C(correctWeights, testWeights, " different weights for " << text);
    }
}

void CreateTokenizersAndTest(ETokenType tokenType, bool needLowerCase, size_t unknownWordId, size_t maxLines, ui64 startId, ui64 hashModulo, EStoreStringType storeStringType) {
    static TVector<TUtf16String> validateTexts = ReadValidateTexts();

    TTermToIndex dict = ReadResourceDict(tokenType, needLowerCase, storeStringType, startId, hashModulo, Max<size_t>());
    TVector<ui64> hashes = GetHashes(dict, storeStringType);
    bool useUnknownWord = true;
    THolder<ITokenizer> correctTokenizer = nullptr;
    if (tokenType == ETokenType::Word) {
        correctTokenizer = MakeHolder<TWordTokenizer>(dict, needLowerCase, useUnknownWord, unknownWordId);
    } else if (tokenType == ETokenType::Bigram) {
        correctTokenizer = MakeHolder<TBigramsTokenizer>(dict, needLowerCase, useUnknownWord, unknownWordId);
    } else if (tokenType == ETokenType::Trigram) {
        correctTokenizer = MakeHolder<TTrigramTokenizer>(dict, needLowerCase, useUnknownWord, unknownWordId);
    } else {
        Y_ENSURE(false, "unsupported tokenType = " << tokenType);
    }
    THolder<TUpdatableDictTokenizer> testTokenizer = MakeHolder<TUpdatableDictTokenizer>(tokenType, hashes, startId, hashModulo, storeStringType, needLowerCase, useUnknownWord, unknownWordId);

    ValidateTokenizer(correctTokenizer.Get(), testTokenizer.Get(), validateTexts, maxLines);
}


Y_UNIT_TEST_SUITE(TUpdatableDictCorrectness) {
    Y_UNIT_TEST(LowerWordCorrectness) {
        for (size_t maxLines : {100, 10'000}) {
            ETokenType tokenType = ETokenType::Word;
            bool needLowerCase = true;
            size_t unknownWordId = 0;
            ui64 startId = 10;
            ui64 hashModulo = 1'000'000;
            EStoreStringType storeStringType = EStoreStringType::UTF16;
            CreateTokenizersAndTest(tokenType, needLowerCase, unknownWordId, maxLines, startId, hashModulo, storeStringType);
        }
    }
    Y_UNIT_TEST(LowerBigramCorrectness) {
        for (size_t maxLines : {100, 10'000}) {
            ETokenType tokenType = ETokenType::Bigram;
            bool needLowerCase = true;
            size_t unknownWordId = 0;
            ui64 startId = 10;
            ui64 hashModulo = 1'000'000;
            EStoreStringType storeStringType = EStoreStringType::UTF16;
            CreateTokenizersAndTest(tokenType, needLowerCase, unknownWordId, maxLines, startId, hashModulo, storeStringType);
        }
    }
    Y_UNIT_TEST(LowerTrigramCorrectness) {
        for (size_t maxLines : {100, 10'000}) {
            ETokenType tokenType = ETokenType::Trigram;
            bool needLowerCase = true;
            size_t unknownWordId = 0;
            ui64 startId = 10;
            ui64 hashModulo = 1'000'000;
            EStoreStringType storeStringType = EStoreStringType::UTF16;
            CreateTokenizersAndTest(tokenType, needLowerCase, unknownWordId, maxLines, startId, hashModulo, storeStringType);
        }
    }
}  // TestCorrectness

Y_UNIT_TEST_SUITE(TUpdatableDictSaveLoad) {
    Y_UNIT_TEST(SaveLoadParams) {
        ETokenType tokenType = ETokenType::Word;
        TVector<ui64> hashes = {0};
        ui64 startId = 10;
        ui64 hashModulo = 1'000'000;
        EStoreStringType storeStringType = EStoreStringType::UTF8;
        bool needLowerCase = true;
        bool useUnknownWord = true;
        size_t unknownWordId = 0;
        TUpdatableDictTokenizer tokenizer(tokenType, hashes, startId, hashModulo, storeStringType, needLowerCase, useUnknownWord, unknownWordId);

        TStringStream ss;
        tokenizer.Save(&ss);
        TBlob blob = TBlob::FromStream(ss);
        {
            TUpdatableDictTokenizer loadedTokenizer;
            UNIT_ASSERT(loadedTokenizer.Load(blob) > 0);

            UNIT_ASSERT_VALUES_EQUAL(loadedTokenizer.NeedLowercaseInput(), needLowerCase);
            UNIT_ASSERT_VALUES_EQUAL(loadedTokenizer.DoUseUnknownWord(), useUnknownWord);
            UNIT_ASSERT_VALUES_EQUAL(loadedTokenizer.GetUnknownWordId(), unknownWordId);

            TVector<size_t> ids;
            TVector<float> values;

            loadedTokenizer.AddTokens(UTF8ToWide("takogoSlovaTochnoNet"), ids, values, nullptr);
            UNIT_ASSERT_VALUES_EQUAL(ids.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(values.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(ids[0], unknownWordId);
        }
    }

    Y_UNIT_TEST(SaveLoadBig) {
        ETokenType tokenType = ETokenType::Word;
        ui64 startId = 10;
        ui64 hashModulo = 1'000'000;
        EStoreStringType storeStringType = EStoreStringType::UTF8;
        bool needLowerCase = true;
        bool useUnknownWord = true;
        size_t unknownWordId = 0;

        TTermToIndex dict = ReadResourceDict(tokenType, needLowerCase, storeStringType, startId, hashModulo, Max<size_t>());
        TVector<ui64> hashes = GetHashes(dict, storeStringType);

        THolder<TUpdatableDictTokenizer> tokenizer = MakeHolder<TUpdatableDictTokenizer>(tokenType, hashes, startId, hashModulo, storeStringType, needLowerCase, useUnknownWord, unknownWordId);

        THolder<TUpdatableDictTokenizer> loadedTokenizer = MakeHolder<TUpdatableDictTokenizer>();

        TStringStream ss;
        tokenizer->Save(&ss);
        TBlob blob = TBlob::FromStream(ss);
        UNIT_ASSERT(loadedTokenizer->Load(blob) > 0);

        TVector<TUtf16String> validation = ReadValidateTexts();

        ValidateTokenizer(tokenizer.Get(), loadedTokenizer.Get(), validation, Max<size_t>());
    }
}  // TestSaveLoad

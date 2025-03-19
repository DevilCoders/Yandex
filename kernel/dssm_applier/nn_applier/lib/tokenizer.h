#pragma once

#include "load_params.h"
#include "compact_types.h"

#include "states.h"
#include "trigram_index.h"
#include "updatable_dicts_index.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <library/cpp/containers/dense_hash/dense_hash.h>
#include <library/cpp/text_processing/dictionary/bpe_dictionary.h>

#include <util/charset/wide.h>
#include <util/generic/vector.h>
#include <util/generic/size_literals.h>
#include <util/memory/blob.h>
#include <util/memory/segmented_string_pool.h>
#include <library/cpp/deprecated/split/split_iterator.h>
#include <util/stream/str.h>
#include <util/ysaveload.h>


namespace NNeuralNetApplier {

void GetWords(const TWtringBuf& wstr, TVector<TWtringBuf>* res) noexcept;

TUtf16String ParseBrokenUTF8(const TString& src);

using TTokenizerUid = TString;

class ITokenizer {
public:
    virtual ~ITokenizer() = default;
    virtual void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const = 0;
    virtual bool NeedLowercaseInput() const = 0;
    virtual TString GetTypeName() const = 0;
    virtual size_t Load(const TBlob& blob) = 0;
    virtual size_t Load(const TBlob& blob, [[maybe_unused]] const TLoadParams& loadParams) {
        return Load(blob);
    }
    virtual void Save(IOutputStream* s) const = 0;

    virtual TTokenizerUid GetUid() const;

    virtual bool DoUseUnknownWord() const = 0;
    virtual size_t GetUnknownWordId() const = 0;
    virtual size_t GetVersion() const { return 0; }
    virtual const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const = 0;
};

using TTermToIndex =THashMap<TUtf16String, size_t>;

class TTrigramTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    TDenseHash<TCompactWtringBuf, TUInt24> Map;
    segmented_pool<wchar16> Pool = segmented_pool<wchar16>(1_MB / sizeof(wchar16));
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;

public:
    TTrigramTokenizer();
    TTrigramTokenizer(const TTermToIndex& mapping, bool lowercaseInput = false,
        bool useUnknownWord = false, size_t unknownWordId = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;
    size_t Load(const TBlob& blob, const TLoadParams& loadParams) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TTrigramTokenizer";
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

class TCachedTrigramTokenizer : public ITokenizer {
private:
    TTrigramsIndex Index;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;

public:
    TCachedTrigramTokenizer();
    TCachedTrigramTokenizer(const TTermToIndex& mapping, bool lowercaseInput = false,
        bool useUnknownWord = false, size_t unknownWordId = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    TString GetTypeName() const override {
        return "TCachedTrigramTokenizer";
    }

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Index.GetTrie();
    }
};

class TWordTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    TDenseHash<TCompactWtringBuf, TUInt24> Map;
    segmented_pool<wchar16> Pool = segmented_pool<wchar16>(1_MB / sizeof(wchar16));
    TSplitDelimiters Delimiters;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;
    size_t Version;

public:
    TWordTokenizer();
    TWordTokenizer(const TTermToIndex& mapping, bool lowercaseInput = false,
        bool useUnknownWord = false, size_t unknownWordId = 0, size_t version = 2);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;
    size_t Load(const TBlob& blob, const TLoadParams& loadParams) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TWordTokenizer";
    }

    TTokenizerUid GetUid() const override {
        return ITokenizer::GetUid() + "_v." + ToString(GetVersion());
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    size_t GetVersion() const override {
        return Version;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

class TPrefixTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    TSplitDelimiters Delimiters;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;

public:
    TPrefixTokenizer();
    TPrefixTokenizer(const TTermToIndex& mapping, bool lowercaseInput = true,
        bool useUnknownWord = false, size_t unknownWordId = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TPrefixTokenizer";
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

class TSuffixTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    TSplitDelimiters Delimiters;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;

public:
    TSuffixTokenizer();
    TSuffixTokenizer(const TTermToIndex& mapping, bool lowercaseInput = true,
        bool useUnknownWord = false, size_t unknownWordId = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TSuffixTokenizer";
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

class TPhraseTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;

public:
    TPhraseTokenizer();
    TPhraseTokenizer(const TTermToIndex& mapping, bool lowercaseInput = false,
        bool useUnknownWord = false, size_t unknownWordId = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TPhraseTokenizer";
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

class TBigramsTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    TDenseHash<TCompactWtringBuf, TUInt24> Map;
    segmented_pool<wchar16> Pool = segmented_pool<wchar16>(1_MB / sizeof(wchar16));
    TSplitDelimiters Delimiters;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;
    size_t Version;

public:
    TBigramsTokenizer();
    TBigramsTokenizer(const TTermToIndex& mapping, bool lowercaseInput = false,
        bool useUnknownWord = false, size_t unknownWordId = 0, size_t version = 2);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;
    size_t Load(const TBlob& blob, const TLoadParams& loadParams) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TBigramsTokenizer";
    }

    TTokenizerUid GetUid() const override {
        return ITokenizer::GetUid() + "_v." + ToString(GetVersion());
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    size_t GetVersion() const override {
        return Version;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

class TWideBigramsTokenizer : public ITokenizer {
private:
    TCompactTrie<TUtf16String::char_type, size_t> Trie;
    TSplitDelimiters Delimiters;
    bool LowercaseInput;
    bool UseUnknownWord;
    size_t UnknownWordId;
    TString Buffer;

public:
    TWideBigramsTokenizer();
    TWideBigramsTokenizer(const TTermToIndex& mapping, bool lowercaseInput = true,
        bool useUnknownWord = false, size_t unknownWordId = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TWideBigramsTokenizer";
    }

    bool DoUseUnknownWord() const override {
        return UseUnknownWord;
    }

    size_t GetUnknownWordId() const override {
        return UnknownWordId;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        return Trie;
    }
};

using NTextProcessing::NDictionary::EUnknownTokenPolicy;
using NTextProcessing::NDictionary::TMMapBpeDictionary;
using NTextProcessing::NDictionary::TMMapDictionary;

class TBpeTokenizer: public ITokenizer {
private:
    TIntrusivePtr<TMMapBpeDictionary> BpeDictionary;
    bool LowercaseInput = true;
    size_t UnknownTokenId = 0;
    EUnknownTokenPolicy UnknownTokenPolicy = EUnknownTokenPolicy::Insert;
    ui64 IdShift = 0;
    size_t Version = 1;

public:
    TBpeTokenizer();
    TBpeTokenizer(TIntrusivePtr<TMMapBpeDictionary> bpeDicitonary, bool lowercaseInput, bool useUnknownWord = true, ui32 idShift = 0);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;

    void AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions = nullptr) const override;

    bool NeedLowercaseInput() const override {
        return LowercaseInput;
    }

    TString GetTypeName() const override {
        return "TBpeTokenizer";
    }

    TTokenizerUid GetUid() const override {
        return ITokenizer::GetUid() + "_v." + ToString(GetVersion());
    }

    bool DoUseUnknownWord() const override {
        return UnknownTokenPolicy == EUnknownTokenPolicy::Insert;
    }

    size_t GetUnknownWordId() const override {
        return UnknownTokenId;
    }

    size_t GetDictionarySize() const {
        return BpeDictionary->Size();
    }

    size_t GetVersion() const override {
        return Version;
    }

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override {
        Y_FAIL("There is no direct token->id dict's representation in TBpeTokenizer");
    }

    static EUnknownTokenPolicy GetUnknownTokenPolicy(bool useUnknownToken) {
        return useUnknownToken == true ? EUnknownTokenPolicy::Insert : EUnknownTokenPolicy::Skip;
    }

    TIntrusivePtr<TMMapBpeDictionary> GetBpeDictionary() const {
        return BpeDictionary;
    }
};


class TUpdatableDictTokenizer : public ITokenizer {
    using THashType = ui64;
public:
    TUpdatableDictTokenizer() = default;

    TUpdatableDictTokenizer(
        ETokenType tokenType,
        const TVector<ui64>& hashes,
        ui64 startId,
        ui64 hashModulo,
        NNeuralNetApplier::EStoreStringType storeStringType,
        bool lowercaseInput = true,
        bool useUnknownWord = false,
        size_t unknownWordId = 0,
        bool allowCollisions = false);

    ~TUpdatableDictTokenizer() override = default;

    void AddTokens(
        const TWtringBuf& w,
        TVector<size_t>& ids,
        TVector<float>& values,
        TPositionedOneHotEncodingVector* positions) const override;

    bool NeedLowercaseInput() const override;

    TString GetTypeName() const override;

    size_t Load(const TBlob& blob) override;

    void Save(IOutputStream* s) const override;

    bool DoUseUnknownWord() const override;

    size_t GetUnknownWordId() const override;

    size_t GetVersion() const override;

    TTokenizerUid GetUid() const override;

    const TCompactTrie<TUtf16String::char_type, size_t>& GetTrie() const override;

private:
    static bool TokenTypeSupported(ETokenType tokenType);

    void AddWordTokens(
        const TWtringBuf& w,
        TVector<size_t>& ids,
        TVector<float>& values,
        TPositionedOneHotEncodingVector* positions) const;

    void AddBigramTokens(
        const TWtringBuf& w,
        TVector<size_t>& ids,
        TVector<float>& values,
        TPositionedOneHotEncodingVector* positions) const;

    void AddTrigramTokens(
        const TWtringBuf& w,
        TVector<size_t>& ids,
        TVector<float>& values,
        TPositionedOneHotEncodingVector* positions) const;

private:
    size_t Version = 1;
    ETokenType TokenType = ETokenType::Word;
    TUpdatableDictIndex<ui64> DictIndex;
    bool LowerCaseInput = true;
    bool UseUnknownWord = false;
    size_t UnknownWordId = 0;
};


class TSparsifier : public IState {
private:
    TVector<TAtomicSharedPtr<ITokenizer>> Tokenizers;

private:
    TString InternalsToString() const;

public:
    TSparsifier() = default;

    TSparsifier(TVector<TAtomicSharedPtr<ITokenizer>> tokenizers);

    void Save(IOutputStream* s) const override;
    size_t Load(const TBlob& blob) override;
    size_t Load(const TBlob& blob, const TLoadParams& loadParams) override;

    void ToSparse(const TString& text, TSparseVector& result, TPositionedOneHotEncodingVector* positions = nullptr);

    const TVector<TAtomicSharedPtr<ITokenizer>>& GetTokenizers() const {
        return Tokenizers;
    }

    TString GetTypeName() const override {
        return "TSparsifier";
    }
};
using TSparsifierPtr = TIntrusivePtr<TSparsifier>;

class TGlobalSparsifier : public IState {
public:
    struct TTokenizerRemap {
        TTokenizerUid Uid;
        size_t Index = 0;

        TTokenizerRemap() = default;
        TTokenizerRemap(const TTokenizerUid& uid, size_t idx)
            : Uid(uid)
            , Index(idx)
        {}

        TString ToString() const;
        void FromString(const TString& str);
    };

    using TPairOfIndexesAndValues = std::pair<TVector<size_t>, TVector<float>>;

private:
    TVector<TAtomicSharedPtr<ITokenizer>> Tokenizers;

    TVector<TBlobArray<ui32>> RemapArrays;
    THashMap<TString, TVector<TTokenizerRemap>> OutputToRemap;

    bool NeedLowercaseInput = false;
    size_t NumTokenizerIds = 0;
    TVector<size_t> TokenizerIds;
    THashMap<TString, size_t> IndexByOutput;
    TVector<TVector<std::pair<size_t, size_t>>> RemapByOutputIndex;

    void DoRemap(const TPairOfIndexesAndValues& input,
        const TBlobArray<ui32>& remapVector, TSparseVector& output) const;

    TString InternalsToString() const;

    void BuildOptimizedMappings();

public:
    TGlobalSparsifier() = default;
    TGlobalSparsifier(const TVector<TAtomicSharedPtr<ITokenizer>>& tokenizers,
                      const THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>>& remap);

    TString GetTypeName() const override {
        return "TGlobalSparsifier";
    }

    void Save(IOutputStream*) const override;
    size_t Load(const TBlob&) override;
    size_t Load(const TBlob& blob, const TLoadParams& loadParams) override;

    void ToSparse(const TString& text, const TVector<std::pair<size_t, TSparseVector&>>& result, TVector<TPairOfIndexesAndValues>& tokenizerResultsBuffer);

    void MapOutputs(const TVector<TString>& outputs, TVector<size_t>& outputIndices) const;

    const TVector<TAtomicSharedPtr<ITokenizer>>& GetTokenizers() const {
        return Tokenizers;
    }

    THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>> DoGetRemap() const;

    void RenameVariable(const TString& name, const TString& newName);
};

using TGlobalSparsifierPtr = TAtomicSharedPtr<TGlobalSparsifier>;

}

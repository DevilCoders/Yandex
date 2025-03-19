#include "tokenizer.h"

#include "saveload_utils.h"
#include "util.h"

#include <util/charset/unidata.h>
#include <util/stream/str.h>
#include <util/string/cast.h>
#include <util/generic/deque.h>
#include <util/generic/hash.h>
#include <util/generic/map.h>

#include <array>

namespace {
    using namespace NNeuralNetApplier;

    template<class TIndex>
    void AddTokenInner(const TWtringBuf& token, ETokenType type, TVector<size_t>& ids, TVector<float>& values,
        TPositionedOneHotEncodingVector* positions, const TIndex& trie,
        const TWtringBuf& source, bool useUnknownWord, size_t unknownWordId, float weight = 1)
    {
        const size_t beginShift = token.begin() - source.begin();
        const size_t endShift = token.end() - source.begin();

        size_t index = 0;
        if (trie.Find(token, &index)) {
            if (positions) {
                positions->emplace_back(index, beginShift, endShift, type);
            }
            ids.push_back(index);
            values.push_back(weight);
        } else if (useUnknownWord) {
            if (positions) {
                positions->emplace_back(unknownWordId, beginShift, endShift, type);
            }
            ids.push_back(unknownWordId);
            values.push_back(weight);
        }
    }

    template<class TIndex>
    void AddTokenInnerMap(const TWtringBuf token, ETokenType type, TVector<size_t>& ids, TVector<float>& values,
        TPositionedOneHotEncodingVector* positions, const TIndex& map,
        const TWtringBuf source, bool useUnknownWord, size_t unknownWordId, float weight = 1)
    {
        const size_t beginShift = token.begin() - source.begin();
        const size_t endShift = token.end() - source.begin();

        ui32 index = 0;
        const TUInt24* indexptr = nullptr;
        if (token.size() <= std::numeric_limits<ui8>::max()) {  // map keys are TCompactWtringBuf
            indexptr = map.FindPtr(token);
        }
        if (indexptr) {
            index = *indexptr;
            if (positions) {
                positions->emplace_back(index, beginShift, endShift, type);
            }
            ids.push_back(index);
            values.push_back(weight);
        } else if (useUnknownWord) {
            if (positions) {
                positions->emplace_back(unknownWordId, beginShift, endShift, type);
            }
            ids.push_back(unknownWordId);
            values.push_back(weight);
        }
    }

    template<class TIndex>
    void AddBigramTokenInner(const TWtringBuf& prevWord, const TWtringBuf& word, ETokenType type,
        TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions,
        const TIndex& trie, const TWtringBuf& source,
        bool useUnknownWord, size_t unknownWordId, float weight = 1)
    {
        const size_t beginShift = prevWord.begin() - source.begin();
        const size_t endShift = word.end() - source.begin();

        TUtf16String curBigram = TUtf16String::Join(prevWord, u" ", word);
        size_t index = 0;
        if (trie.Find(curBigram, &index)) {
            if (positions) {
                positions->emplace_back(index, beginShift, endShift, type);
            }
            ids.push_back(index);
            values.push_back(weight);
        } else if (useUnknownWord) {
            if (positions) {
                positions->emplace_back(unknownWordId, beginShift, endShift, type);
            }
            ids.push_back(unknownWordId);
            values.push_back(weight);
        }
    }

    template<class TIndex>
    void AddBigramTokenInnerMap(const TWtringBuf prevWord, const TWtringBuf word, ETokenType type,
        TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions,
        const TIndex& map, const TWtringBuf source,
        bool useUnknownWord, size_t unknownWordId, float weight = 1)
    {
        const size_t beginShift = prevWord.begin() - source.begin();
        const size_t endShift = word.end() - source.begin();

        TUtf16String curBigram = TUtf16String::Join(prevWord, u" ", word);
        ui32 index = 0;
        const TUInt24* indexptr = nullptr;
        if (curBigram.size() <= std::numeric_limits<ui8>::max()) {  // map keys are TCompactWtringBuf
            indexptr = map.FindPtr(curBigram);
        }
        if (indexptr) {
            index = *indexptr;
            if (positions) {
                positions->emplace_back(index, beginShift, endShift, type);
            }
            ids.push_back(index);
            values.push_back(weight);
        } else if (useUnknownWord) {
            if (positions) {
                positions->emplace_back(unknownWordId, beginShift, endShift, type);
            }
            ids.push_back(unknownWordId);
            values.push_back(weight);
        }
    }

    void ProcessWideBigrams(const TDeque<TWtringBuf>& words, TVector<size_t>& ids, TVector<float>& values,
        TPositionedOneHotEncodingVector* positions, const TCompactTrie<TUtf16String::char_type, size_t>& trie,
        const TWtringBuf& source, bool useUnknownWord, size_t unknownWordId)
    {
        size_t nwords = words.size();
        constexpr size_t maxNWords = 5;

        if (Y_UNLIKELY(nwords > maxNWords)) {
            Y_ASSERT(false);
            nwords = maxNWords;
        }

        const auto& first = words.at(0);
        for (size_t i = 1; i < nwords; ++i) {
            constexpr std::array<float, 4> weights = {{1, 0.8, 0.6, 0.4}};

            AddBigramTokenInner(first, words.at(i), ETokenType::WideBigram, ids, values, positions,
                trie, source, useUnknownWord, unknownWordId, weights[i - 1]);
        }
    }

    void PackSparseVector(TSparseVector& result) {
        if (result.Indexes.empty()) {
            return;
        }
        NNeuralNetApplier::SortIndices<ui32, true>(result.Indexes, result.Values);
        size_t nTokens = 0;
        size_t currentIndex = result.Indexes[0];
        float count = 0;
        for (size_t i = 0; i < result.Indexes.size(); ++i) {
            size_t idx = result.Indexes[i];
            if (idx != currentIndex) {
                result.Indexes[nTokens] = currentIndex;
                result.Values[nTokens] = count;
                currentIndex = idx;
                count = 0;
                ++nTokens;
            }
            count += result.Values[i];
        }
        result.Indexes[nTokens] = currentIndex;
        result.Values[nTokens] = count;
        result.Indexes.resize(nTokens + 1);
        result.Values.resize(nTokens + 1);
    }
};

namespace NBpeUtils {

    using TBpeDicitonaryApplier = NTextProcessing::NDictionary::TMMapBpeDictionary;
    using TAlphabetApplier = NTextProcessing::NDictionary::TMMapDictionary;

    inline TString SerializeBpeDictionaryApplier(const TIntrusivePtr<TBpeDicitonaryApplier>& bpeDictionary) {
        TStringStream bpeSerializedStream;
        TStringStream alphabetStream;
        TStringStream bpeUnitsStream;
        bpeDictionary->GetAlphabet()->Save(&alphabetStream);
        bpeDictionary->Save(&bpeUnitsStream);
        SaveString64(&bpeSerializedStream, alphabetStream.Str());
        SaveString64(&bpeSerializedStream, bpeUnitsStream.Str());
        return bpeSerializedStream.Str();
    }

    inline void LoadBpeDictionaryApplier(TBlob& blob, const TIntrusivePtr<TBpeDicitonaryApplier>& bpeDictionary) {
        auto mmapAlphabet = MakeIntrusive<TAlphabetApplier>();
        auto alphabetBlob = NNeuralNetApplier::ReadBlob(blob);
        mmapAlphabet->InitFromMemory(alphabetBlob.AsCharPtr(), alphabetBlob.Size());
        bpeDictionary->SetAlphabet(mmapAlphabet);
        auto bpeUnitsBlob = NNeuralNetApplier::ReadBlob(blob);
        bpeDictionary->InitFromMemory(bpeUnitsBlob.AsCharPtr(), bpeUnitsBlob.Size());
    }

}

namespace NNeuralNetApplier {

void GetWords(const TWtringBuf& wstr, TVector<TWtringBuf>* res) noexcept {
    res->clear();

    auto start = wstr.end();
    auto end = wstr.end();

    const auto dump = [&] () {
        if (start != wstr.end()) {
            res->push_back(TWtringBuf(start, end));
        }
        start = end = wstr.end();
    };

    for (auto ch = wstr.begin(); ch != wstr.end(); ++ch) {
        if (IsAlpha(*ch) || IsDigit(*ch)) {
            if (start == wstr.end()) {
                start = end = ch;
            }
            ++end;
        } else {
            dump();
        }
    }
    dump();
}

TUtf16String ParseBrokenUTF8(const TString& src) {
    TVector<ui8> dstBufVec(src.size());
    ui8* dPtr = dstBufVec.data();
    const ui8* sPtr = (const ui8*)src.begin();
    const ui8* sEnd = (const ui8*)src.end();
    while (sPtr < sEnd) {
        wchar32 rune;
        size_t sRuneLen;
        if (SafeReadUTF8Char(rune, sRuneLen, sPtr, sEnd) != RECODE_OK) {
            ++sPtr;
            continue;
        }
        size_t dRuneLen;
        if (SafeWriteUTF8Char(rune, dRuneLen, dPtr, dstBufVec.end()) != RECODE_OK) {
            break;
        }
        Y_ENSURE(dRuneLen == sRuneLen);
        dPtr += dRuneLen;
        sPtr += sRuneLen;
    }
    return UTF8ToWide(TStringBuf((const char*)dstBufVec.data(), (const char*)dPtr));
}

class TWordExtractor {
private:
    TWtringBuf Wstr;
    const wchar16* Start;
    const wchar16* End;
    const wchar16* Ptr;

public:
    TWordExtractor(const TWtringBuf& wstr)
        : Wstr(wstr)
        , Start(Wstr.end())
        , End(Wstr.end())
        , Ptr(wstr.begin())
    {}

    bool Next(TWtringBuf& s) {
        for (; Ptr != Wstr.end(); ++Ptr) {
            if (IsAlpha(*Ptr) || IsDigit(*Ptr)) {
                if (Start == Wstr.end()) {
                    Start = End = Ptr;
                }
                ++End;
            } else {
                if (Start != Wstr.end()) {
                    s = TWtringBuf(Start, End);
                    Start = End = Wstr.end();
                    return true;
                }
                Start = End = Wstr.end();
            }
        }
        if (Start != Wstr.end()) {
            s = TWtringBuf(Start, End);
            Start = End = Wstr.end();
            return true;
        }
        return false;
    }
};

ITokenizer* CreateTokenizer(const TString& tokenizerType) {
    if (tokenizerType == "TTrigramTokenizer") {
        return new TTrigramTokenizer();
    } else if (tokenizerType == "TWordTokenizer") {
        return new TWordTokenizer();
    } else if (tokenizerType == "TPhraseTokenizer") {
        return new TPhraseTokenizer();
    } else if (tokenizerType == "TBigramsTokenizer") {
        return new TBigramsTokenizer();
    } else if (tokenizerType == "TCachedTrigramTokenizer") {
        return new TCachedTrigramTokenizer();
    } else if (tokenizerType == "TWideBigramsTokenizer") {
        return new TWideBigramsTokenizer();
    } else if (tokenizerType == "TPrefixTokenizer") {
        return new TPrefixTokenizer();
    } else if (tokenizerType == "TSuffixTokenizer") {
        return new TSuffixTokenizer();
    } else if (tokenizerType == "TBpeTokenizer") {
        return new TBpeTokenizer();
    } else if (tokenizerType == "TUpdatableDictTokenizer") {
        return new TUpdatableDictTokenizer();
    }

    ythrow yexception() << "Unknown tokenizer type: " << tokenizerType;
}

void LoadIndexFromBlob(TCompactTrie<TUtf16String::char_type, size_t>& trie, TBlob& blob) {
    trie.Init(blob);
}

void LoadIndexFromBlob(TTrigramsIndex& index, TBlob& blob) {
    index.Init(blob);
}

void LoadIndexFromBlob(const TIntrusivePtr<TMMapBpeDictionary>& bpeDictionary, TBlob& blob) {
    NBpeUtils::LoadBpeDictionaryApplier(blob, bpeDictionary);
}

template <class T>
void LoadIndexFromBlob(TUpdatableDictIndex<T>& index, TBlob& blob) {
    return index.Init(blob);
}

template <class TIndex>
size_t LoadTokenizer(
    const TBlob& blob,
    TIndex& index,
    bool& lowercaseInput,
    bool& useUnknownWord,
    size_t& unknownWordId,
    size_t& version,
    THashMap<TString, TString>* extractedFields = nullptr)
{
    TBlob curBlob = blob;
    size_t totalLength = ReadSize(curBlob);
    curBlob = curBlob.SubBlob(0, totalLength);

    THashMap<TString, TString> fields = ReadFields(curBlob);
    if (extractedFields) {
        *extractedFields = fields;
    }
    lowercaseInput = FromString<bool>(fields.at("LowercaseInput"));
    useUnknownWord = FromString<bool>(fields.at("UseUnknownWord"));
    unknownWordId = FromString<ui64>(fields.at("UnknownWordId"));

    if (fields.contains("Version")) {
        version = FromString<ui64>(fields.at("Version"));
    } else {
        version = 1;
    }

    TBlob indexBlob = ReadBlob(curBlob);
    LoadIndexFromBlob(index, indexBlob);
    Y_ASSERT(curBlob.Empty());

    return totalLength + sizeof(size_t);
}

template <class T>
TString IndexToString(const T& index) {
    TStringStream ss;
    index.Save(&ss);
    return ss.Str();
}

TString IndexToString(const TIntrusivePtr<TMMapBpeDictionary>& bpeDictionary) {
    return NBpeUtils::SerializeBpeDictionaryApplier(bpeDictionary);
}

TString IndexToString(const TCompactTrie<TUtf16String::char_type, size_t>& trie) {
    return TrieToString(trie);
}

template <class TIndex>
void SaveTokenizer(
    const TIndex& index,
    const bool lowercaseInput,
    const bool useUnknownWord,
    const size_t unknownWordId,
    const size_t version,
    const THashMap<TString, TString>& additionalFields,
    IOutputStream* s)
{
    TString indexString = IndexToString(index);
    THashMap<TString, TString> fields = additionalFields;
    fields["LowercaseInput"] = ToString(lowercaseInput);
    fields["UseUnknownWord"] = ToString(useUnknownWord);
    fields["UnknownWordId"] = ToString(unknownWordId);
    fields["Version"] = ToString(version);

    TString fieldsString = SaveToStroka(fields);
    SaveVectorStrok(s, { fieldsString, indexString });
}

template <class TIndex>
void SaveTokenizer(const TIndex& index,
    const bool lowercaseInput, const bool useUnknownWord, const size_t unknownWordId,
    const size_t version, IOutputStream* s)
{
    SaveTokenizer(index, lowercaseInput, useUnknownWord, unknownWordId, version, {}, s);
}

TTokenizerUid ITokenizer::GetUid() const {
    return TString::Join(GetTypeName()
        , '_', (NeedLowercaseInput() ? TStringBuf("l") : TStringBuf("nl"))
        , '_', (DoUseUnknownWord() ? TStringBuf("uuw") : TStringBuf("nuuw"))
        , '_', ToString(GetUnknownWordId()));
}

template <class T>
void DoAddTrigramTokens(const T& indexData,
    bool useUnknownWord, size_t unknownWordId,
    const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values,
    TPositionedOneHotEncodingVector* positions)
{
    size_t numTrigrams = w.size() < 3 ? 0 : w.size() - 2;
    for (size_t i = 0; i < numTrigrams; i++) {
        TWtringBuf trigram = w.substr(i, 3);
        size_t index = 0;
        if (indexData.Find(trigram, &index)) {
            if (positions) {
                positions->emplace_back(index, i, i + 3, ETokenType::Trigram);
            }
            ids.push_back(index);
            values.push_back(1.f);
        } else if (useUnknownWord) {
            if (positions) {
                positions->emplace_back(unknownWordId, i, i + 3, ETokenType::Trigram);
            }
            ids.push_back(unknownWordId);
            values.push_back(1.f);
        }
    }
}

template <class TIndex>
void DoAddTrigramTokensMap(const TIndex& map,
    bool useUnknownWord, size_t unknownWordId,
    const TWtringBuf w, TVector<size_t>& ids, TVector<float>& values,
    TPositionedOneHotEncodingVector* positions)
{
    size_t numTrigrams = w.size() < 3 ? 0 : w.size() - 2;
    for (size_t i = 0; i < numTrigrams; i++) {
        TWtringBuf trigram = w.substr(i, 3);
        ui32 index = 0;
        if (const TUInt24* indexptr = map.FindPtr(trigram); indexptr) {
            index = *indexptr;
            if (positions) {
                positions->emplace_back(index, i, i + 3, ETokenType::Trigram);
            }
            ids.push_back(index);
            values.push_back(1.f);
        } else if (useUnknownWord) {
            if (positions) {
                positions->emplace_back(unknownWordId, i, i + 3, ETokenType::Trigram);
            }
            ids.push_back(unknownWordId);
            values.push_back(1.f);
        }
    }
}

TTrigramTokenizer::TTrigramTokenizer()
    : LowercaseInput(false)
    , UseUnknownWord(false)
    , UnknownWordId(0)
{
}

TTrigramTokenizer::TTrigramTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId)
    : LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TTrigramTokenizer::Load(const TBlob& blob) {
    return Load(blob, {});
}

size_t TTrigramTokenizer::Load(const TBlob& blob, const TLoadParams& loadParams) {
    size_t version;
    size_t size = LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, version);
    if (loadParams.UseHashMapTokenizersOptimization) {
        Map.ReserveSpace(Trie.Size());
        for (auto entry : Trie) {
            auto begin = Pool.append(entry.first.data(), entry.first.size());
            Map[TCompactWtringBuf(begin, entry.first.size())] = entry.second;
        }
    }
    return size;
}

void TTrigramTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, 1, s);
}

//assume UTF-16 encodes all chars in 2 bytes
void TTrigramTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    if (!Map.Empty()) {
        DoAddTrigramTokensMap(Map, UseUnknownWord, UnknownWordId, w, ids, values, positions);
    } else {
        DoAddTrigramTokens(Trie, UseUnknownWord, UnknownWordId, w, ids, values, positions);
    }
}

TCachedTrigramTokenizer::TCachedTrigramTokenizer()
    : LowercaseInput(false)
    , UseUnknownWord(false)
    , UnknownWordId(0)
{
}

TCachedTrigramTokenizer::TCachedTrigramTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId)
    : Index(mapping)
    , LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
}

size_t TCachedTrigramTokenizer::Load(const TBlob& blob) {
    size_t version;
    return LoadTokenizer(blob, Index, LowercaseInput, UseUnknownWord, UnknownWordId, version);
}

void TCachedTrigramTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Index, LowercaseInput, UseUnknownWord, UnknownWordId, 1, s);
}

//assume UTF-16 encodes all chars in 2 bytes
void TCachedTrigramTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    DoAddTrigramTokens(Index, UseUnknownWord, UnknownWordId, w, ids, values, positions);
}

TWordTokenizer::TWordTokenizer()
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(false)
    , UseUnknownWord(false)
    , UnknownWordId(0)
    , Version(2)
{

}

TWordTokenizer::TWordTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId, size_t version)
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
    , Version(version)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TWordTokenizer::Load(const TBlob& blob) {
    return Load(blob, {});
}

size_t TWordTokenizer::Load(const TBlob& blob, const TLoadParams& loadParams) {
    size_t size = LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, Version);
    if (loadParams.UseHashMapTokenizersOptimization) {
        Map.ReserveSpace(Trie.Size());
        for (auto entry : Trie) {
            auto begin = Pool.append(entry.first.data(), entry.first.size());
            Map[TCompactWtringBuf(begin, entry.first.size())] = entry.second;
        }
    }
    return size;
}

void TWordTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, Version, s);
}

TVector<size_t> GetUTF8ToUTF16PositionMap(const TString& s) {
    TVector<size_t> result(s.size() + 1, 0);
    size_t u8Position = 0, u16Position = 0;
    while (u8Position < s.size()) {
        size_t runeLen = UTF8RuneLen(s[u8Position]);
        if (!runeLen || u8Position + runeLen >= result.size()) {
            return result;
        }
        u8Position += runeLen;
        if (runeLen < 4) {
            u16Position += 1;
        } else {
            u16Position += 2;
        }
        result[u8Position] = u16Position;
    }
    return result;
}

void TWordTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    if (Version == 1) {
        TString lstr = WideToUTF8(w);
        TVector<size_t> utf16Positions;
        if (positions) {
            //utf16->utf8, then essentially utf8->utf16, is it necessary?
            utf16Positions = GetUTF8ToUTF16PositionMap(lstr);
        }
        TDelimitersSplit Splitter(lstr, Delimiters);
        auto i = Splitter.Iterator();
        TSizeTRegion wordRegion;
        while (!i.Eof()) {
            wordRegion = i.Next();
            TUtf16String curWord = UTF8ToWide(TStringBuf(lstr.data() + wordRegion.Begin, wordRegion.End - wordRegion.Begin));
            size_t index = 0;
            if (Trie.Find(curWord, &index)) {
                if (positions) {
                    positions->emplace_back(index, utf16Positions[wordRegion.Begin], utf16Positions[wordRegion.End], ETokenType::Word);
                }
                ids.push_back(index);
                values.push_back(1.f);
            } else if (UseUnknownWord) {
                if (positions) {
                    positions->emplace_back(UnknownWordId, utf16Positions[wordRegion.Begin], utf16Positions[wordRegion.End], ETokenType::Word);
                }
                ids.push_back(UnknownWordId);
                values.push_back(1.f);
            }
        }
    } else {
        TWtringBuf wstr;
        TWordExtractor extractor(w);
        while (extractor.Next(wstr)) {
            if (!Map.Empty()) {
                AddTokenInnerMap(wstr, ETokenType::Word, ids, values, positions, Map, w, UseUnknownWord, UnknownWordId);
            } else {
                AddTokenInner(wstr, ETokenType::Word, ids, values, positions, Trie, w, UseUnknownWord, UnknownWordId);
            }
        }
    }
}

TPrefixTokenizer::TPrefixTokenizer()
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(true)
    , UseUnknownWord(false)
    , UnknownWordId(0)
{

}

TPrefixTokenizer::TPrefixTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId)
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TPrefixTokenizer::Load(const TBlob& blob) {
    size_t version;
    return LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, version);
}

void TPrefixTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, 1, s);
}

void TPrefixTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    TWtringBuf wstr;
    TWordExtractor extractor(w);
    while (extractor.Next(wstr)) {
        for (size_t len = 3; len < wstr.length(); ++len) {
            TWtringBuf prefix(wstr.begin(), len);
            AddTokenInner(prefix, ETokenType::Prefix, ids, values, positions, Trie, w, UseUnknownWord, UnknownWordId);
        }
    }
}

TSuffixTokenizer::TSuffixTokenizer()
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(true)
    , UseUnknownWord(false)
    , UnknownWordId(0)
{

}

TSuffixTokenizer::TSuffixTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId)
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TSuffixTokenizer::Load(const TBlob& blob) {
    size_t version;
    return LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, version);
}

void TSuffixTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, 1, s);
}

void TSuffixTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    TWtringBuf wstr;
    TWordExtractor extractor(w);
    while (extractor.Next(wstr)) {
        for (size_t len = 3; len < wstr.length(); ++len) {
            TWtringBuf suffix(wstr.end() - len, len);
            AddTokenInner(suffix, ETokenType::Suffix, ids, values, positions, Trie, w, UseUnknownWord, UnknownWordId);
        }
    }
}

TPhraseTokenizer::TPhraseTokenizer()
    : LowercaseInput(false)
    , UseUnknownWord(false)
    , UnknownWordId(0)
{
}

TPhraseTokenizer::TPhraseTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId)
    : LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TPhraseTokenizer::Load(const TBlob& blob) {
    size_t version;
    return LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, version);
}

void TPhraseTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, 1, s);
}

void TPhraseTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    size_t index = 0;
    if (Trie.Find(w, &index)) {
        if (positions) {
            positions->emplace_back(index, 0, w.size(), ETokenType::Phrase);
        }
        ids.push_back(index);
        values.push_back(1.f);
    } else if (UseUnknownWord) {
        if (positions) {
            positions->emplace_back(UnknownWordId, 0, w.size(), ETokenType::Phrase);
        }
        ids.push_back(UnknownWordId);
        values.push_back(1.f);
    }
}

TBigramsTokenizer::TBigramsTokenizer()
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(false)
    , UseUnknownWord(false)
    , UnknownWordId(0)
    , Version(2)
{
}

TBigramsTokenizer::TBigramsTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId, size_t version)
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
    , Version(version)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TBigramsTokenizer::Load(const TBlob& blob) {
    return Load(blob, {});
}

size_t TBigramsTokenizer::Load(const TBlob& blob, const TLoadParams& loadParams) {
    size_t size = LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, Version);
    if (loadParams.UseHashMapTokenizersOptimization) {
        Map.ReserveSpace(Trie.Size());
        for (auto entry : Trie) {
            auto begin = Pool.append(entry.first.data(), entry.first.size());
            Map[TCompactWtringBuf(begin, entry.first.size())] = entry.second;
        }
    }
    return size;
}

void TBigramsTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, Version, s);
}

void TBigramsTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    if (Version == 1) {
        TString lstr = WideToUTF8(w);
        TVector<size_t> utf16Positions;
        if (positions) {
            //utf16->utf8, then essentially utf8->utf16, is it necessary?
            utf16Positions = GetUTF8ToUTF16PositionMap(lstr);
        }
        TDelimitersSplit Splitter(lstr, Delimiters);
        auto i = Splitter.Iterator();
        TUtf16String prevWord;
        TSizeTRegion wordRegion;
        size_t prevBegin = 0;
        while (!i.Eof()) {
            wordRegion = i.Next();
            TUtf16String curWord = UTF8ToWide(TStringBuf(lstr.data() + wordRegion.Begin, wordRegion.End - wordRegion.Begin));
            TUtf16String curBigram = prevWord + u" " + curWord;
            if (prevWord.size()) {
                size_t index = 0;
                if (Trie.Find(curBigram, &index)) {
                    if (positions) {
                        positions->emplace_back(index, utf16Positions[prevBegin], utf16Positions[wordRegion.End], ETokenType::Bigram);
                    }
                    ids.push_back(index);
                    values.push_back(1.f);
                } else if (UseUnknownWord) {
                    if (positions) {
                        positions->emplace_back(UnknownWordId, utf16Positions[prevBegin], utf16Positions[wordRegion.End], ETokenType::Bigram);
                    }
                    ids.push_back(UnknownWordId);
                    values.push_back(1.f);
                }
            }
            prevWord = curWord;
            prevBegin = wordRegion.Begin;
        }
    } else {
        TWtringBuf prevWord;
        TWtringBuf word;
        TWordExtractor extractor(w);
        while (extractor.Next(word)) {
            if (prevWord.size()) {
                if (!Map.Empty()) {
                    AddBigramTokenInnerMap(prevWord, word, ETokenType::Bigram, ids, values, positions, Map, w, UseUnknownWord, UnknownWordId);
                } else {
                    AddBigramTokenInner(prevWord, word, ETokenType::Bigram, ids, values, positions, Trie, w, UseUnknownWord, UnknownWordId);
                }
            }
            prevWord = word;
        }
    }
}

TWideBigramsTokenizer::TWideBigramsTokenizer()
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(true)
    , UseUnknownWord(false)
    , UnknownWordId(0)
{
}

TWideBigramsTokenizer::TWideBigramsTokenizer(const TTermToIndex& mapping, bool lowercaseInput,
    bool useUnknownWord, size_t unknownWordId)
    : Delimiters("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~\t\n \x07")
    , LowercaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
    Buffer = HashMapToTrieString(mapping);
    TStringStream stream(Buffer);
    Trie.Init(TBlob::FromStream(stream));
}

size_t TWideBigramsTokenizer::Load(const TBlob& blob) {
    size_t version;
    return LoadTokenizer(blob, Trie, LowercaseInput, UseUnknownWord, UnknownWordId, version);
}

void TWideBigramsTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(Trie, LowercaseInput, UseUnknownWord, UnknownWordId, 1, s);
}

void TWideBigramsTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    TDeque<TWtringBuf> words;
    TWtringBuf word;

    TWordExtractor extractor(w);
    while (extractor.Next(word)) {
        words.push_back(word);
        if (words.size() == 5) {
            ProcessWideBigrams(words, ids, values, positions, Trie, w, UseUnknownWord, UnknownWordId);
            words.pop_front();
        }
    }
    while (words.size() > 1) {
        ProcessWideBigrams(words, ids, values, positions, Trie, w, UseUnknownWord, UnknownWordId);
        words.pop_front();
    }
}

TBpeTokenizer::TBpeTokenizer()
    : LowercaseInput(false)
    , UnknownTokenId(0)
    , UnknownTokenPolicy(GetUnknownTokenPolicy(true))
    , Version(1)
{
}

TBpeTokenizer::TBpeTokenizer(TIntrusivePtr<TMMapBpeDictionary> bpeDictionary, bool lowerCaseInput, bool useUnknownWord, ui32 idShift)
    : BpeDictionary(std::move(bpeDictionary))
    , LowercaseInput(lowerCaseInput)
    , UnknownTokenId(BpeDictionary->GetUnknownTokenId())
    , UnknownTokenPolicy(GetUnknownTokenPolicy(useUnknownWord))
    , IdShift(idShift)
{
}

void TBpeTokenizer::Save(IOutputStream* s) const {
    SaveTokenizer(BpeDictionary, false, DoUseUnknownWord(), UnknownTokenId, Version, s);
    ::Save(s, IdShift);
}

size_t TBpeTokenizer::Load(const TBlob& blob) {
    bool useUnknownToken;
    bool lowercaseInput;
    BpeDictionary = MakeIntrusive<TMMapBpeDictionary>();
    auto loadedSize = LoadTokenizer(blob, BpeDictionary, lowercaseInput, useUnknownToken, UnknownTokenId, Version);
    UnknownTokenPolicy = GetUnknownTokenPolicy(useUnknownToken);
    IdShift = ReadUnaligned<ui64>(blob.Begin() + loadedSize);
    return loadedSize + sizeof(ui64);
}

void TBpeTokenizer::AddTokens(const TWtringBuf& w, TVector<size_t>& ids, TVector<float>& values, TPositionedOneHotEncodingVector* positions) const {
    TString utf8Text = WideToUTF8(ToWtring(w));
    TVector<ui32> tokenIds;
    BpeDictionary->Apply(TVector<TStringBuf>({utf8Text}), &tokenIds, UnknownTokenPolicy);
    for (size_t tokenId : tokenIds) {
        ids.push_back(tokenId + IdShift);
        values.push_back(1.f);
        if (positions != nullptr) {
            Y_FAIL("Adding token positions to TPositionedOneHotEncodingVector is unsupported by TMMapBpeTokenizer");
        }
    }
}


TUpdatableDictTokenizer::TUpdatableDictTokenizer(
    ETokenType tokenType,
    const TVector<ui64>& hashes,
    ui64 startId,
    ui64 hashModulo,
    NNeuralNetApplier::EStoreStringType storeStringType,
    bool lowercaseInput,
    bool useUnknownWord,
    size_t unknownWordId,
    bool allowCollisions)
    : TokenType(tokenType)
    , DictIndex(hashes, startId, hashModulo, storeStringType, allowCollisions)
    , LowerCaseInput(lowercaseInput)
    , UseUnknownWord(useUnknownWord)
    , UnknownWordId(unknownWordId)
{
    Y_ENSURE(TokenTypeSupported(tokenType), "TUpdatableDictTokenizer is not defined for TokenType = " << ToString(TokenType));
}


void TUpdatableDictTokenizer::AddTokens(
    const TWtringBuf& w,
    TVector<size_t>& ids,
    TVector<float>& values,
    TPositionedOneHotEncodingVector* positions) const
{
    if (TokenType == ETokenType::Word) {
        AddWordTokens(w, ids, values, positions);
    } else if (TokenType == ETokenType::Bigram) {
        AddBigramTokens(w, ids, values, positions);
    } else if (TokenType == ETokenType::Trigram) {
        AddTrigramTokens(w, ids, values, positions);
    } else {
        Y_ENSURE(false);
    }
}

void TUpdatableDictTokenizer::AddWordTokens(
    const TWtringBuf& w,
    TVector<size_t>& ids,
    TVector<float>& values,
    TPositionedOneHotEncodingVector* positions) const
{
    TWtringBuf wstr;
    TWordExtractor extractor(w);
    while (extractor.Next(wstr)) {
        AddTokenInner(wstr, ETokenType::Word, ids, values, positions, DictIndex, w, UseUnknownWord, UnknownWordId);
    }
}

void TUpdatableDictTokenizer::AddBigramTokens(
    const TWtringBuf& w,
    TVector<size_t>& ids,
    TVector<float>& values,
    TPositionedOneHotEncodingVector* positions) const
{
    TWtringBuf prevWord;
    TWtringBuf word;
    TWordExtractor extractor(w);
    while (extractor.Next(word)) {
        if (!prevWord.empty()) {
            AddBigramTokenInner(prevWord, word, ETokenType::Bigram, ids, values, positions, DictIndex, w, UseUnknownWord, UnknownWordId);
        }
        prevWord = word;
    }
}

void TUpdatableDictTokenizer::AddTrigramTokens(
    const TWtringBuf& w,
    TVector<size_t>& ids,
    TVector<float>& values,
    TPositionedOneHotEncodingVector* positions) const
{
    DoAddTrigramTokens(DictIndex, UseUnknownWord, UnknownWordId, w, ids, values, positions);
}

bool TUpdatableDictTokenizer::NeedLowercaseInput() const {
    return LowerCaseInput;
}

TString TUpdatableDictTokenizer::GetTypeName() const {
    return "TUpdatableDictTokenizer";
}

size_t TUpdatableDictTokenizer::Load(const TBlob& blob) {
    THashMap<TString, TString> fields;
    size_t bytesLoaded = LoadTokenizer(blob, DictIndex, LowerCaseInput, UseUnknownWord, UnknownWordId, Version, &fields);
    TokenType = FromString<ETokenType>(fields.at("TokenType"));
    return bytesLoaded;
}

void TUpdatableDictTokenizer::Save(IOutputStream* os) const {
    THashMap<TString, TString> additionalFields = {
        {"TokenType", ToString(TokenType)},
    };
    SaveTokenizer(DictIndex, LowerCaseInput, UseUnknownWord, UnknownWordId, Version, additionalFields, os);
}

bool TUpdatableDictTokenizer::DoUseUnknownWord() const {
    return UseUnknownWord;
}

size_t TUpdatableDictTokenizer::GetUnknownWordId() const {
    return UnknownWordId;
}

size_t TUpdatableDictTokenizer::GetVersion() const {
    return Version;
}

bool TUpdatableDictTokenizer::TokenTypeSupported(ETokenType tokenType) {
    static constexpr std::array SupportedTokenTypes = {
        ETokenType::Word,
        ETokenType::Bigram,
        ETokenType::Trigram,
    };
    return FindPtr(SupportedTokenTypes, tokenType) != nullptr;
}

const TCompactTrie<TUtf16String::char_type, size_t>& TUpdatableDictTokenizer::GetTrie() const {
    Y_FAIL("There is no direct token -> id dict's representation in TUpdatableDictTokenizer");
    static TCompactTrie<TUtf16String::char_type, size_t> trie;
    return trie;
}

TTokenizerUid TUpdatableDictTokenizer::GetUid() const {
    return TString::Join(ITokenizer::GetUid(), "_", ToString(TokenType));
}


TString TSparsifier::InternalsToString() const {
    TStringStream s;
    ::Save(&s, Tokenizers.size());
    for (auto& tok : Tokenizers) {
        TString tokenizerName = tok->GetTypeName();
        ::Save(&s, tokenizerName.size());
        s << tokenizerName;
        tok->Save(&s);
    }
    return TString(s.Data(), s.Size());
}

TSparsifier::TSparsifier(TVector<TAtomicSharedPtr<ITokenizer>> tokenizers)
    : Tokenizers(std::move(tokenizers))
{
}

size_t TSparsifier::Load(const TBlob& blob) {
    TBlob curBlob = blob;
    size_t totalLenth = ReadSize(curBlob);

    curBlob = curBlob.SubBlob(0, totalLenth);

    size_t numTokenizers = ReadSize(curBlob);
    for (size_t i = 0; i < numTokenizers; ++i) {
        TString tokenizerType = ReadString(curBlob);
        ITokenizer* tokenizer = CreateTokenizer(tokenizerType);
        size_t loaded = tokenizer->Load(curBlob);
        curBlob = curBlob.SubBlob(loaded, curBlob.Size());
        Tokenizers.push_back(tokenizer);
    }

    Y_ASSERT(curBlob.Empty());

    return totalLenth + sizeof(size_t);
}

size_t TSparsifier::Load(const TBlob& blob, const TLoadParams& loadParams) {
    TBlob curBlob = blob;
    size_t totalLenth = ReadSize(curBlob);

    curBlob = curBlob.SubBlob(0, totalLenth);

    size_t numTokenizers = ReadSize(curBlob);
    for (size_t i = 0; i < numTokenizers; ++i) {
        TString tokenizerType = ReadString(curBlob);
        ITokenizer* tokenizer = CreateTokenizer(tokenizerType);
        size_t loaded = tokenizer->Load(curBlob, loadParams);
        curBlob = curBlob.SubBlob(loaded, curBlob.Size());
        Tokenizers.push_back(tokenizer);
    }

    Y_ASSERT(curBlob.Empty());

    return totalLenth + sizeof(size_t);
}

void TSparsifier::Save(IOutputStream* s) const {
    TString internals = InternalsToString();
    ::Save(s, internals.size());
    *s << internals;
}

void TSparsifier::ToSparse(const TString& text, TSparseVector& result, TPositionedOneHotEncodingVector* positions) {
    TUtf16String wTextStr;
    try {
        wTextStr = UTF8ToWide(text);
    } catch (const yexception&) {
        wTextStr = ParseBrokenUTF8(text);
    }
    TUtf16String lowerWTextStr;

    bool needLowercase = false;
    for (auto& tokenizer : Tokenizers) {
        needLowercase |= tokenizer->NeedLowercaseInput();
    }

    if (needLowercase) {
        lowerWTextStr = wTextStr;
        lowerWTextStr.to_lower();
    }

    result.Indexes.clear();
    result.Values.clear();
    for (auto& tokenizer : Tokenizers) {
        const TUtf16String& input = tokenizer->NeedLowercaseInput() ? lowerWTextStr : wTextStr;
        tokenizer->AddTokens(input, result.Indexes, result.Values, positions);
    }

    PackSparseVector(result);
}

TGlobalSparsifier::TGlobalSparsifier(const TVector<TAtomicSharedPtr<ITokenizer>>& tokenizers,
    const THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>>& remap)
    : Tokenizers(tokenizers)
{
    TMap<TVector<ui32>, size_t> set;
    size_t idx = 0;
    for (const auto& item : remap) {
        for (const auto& pair : item.second) {
            if (!set.contains(pair.second)) {
                RemapArrays.emplace_back();
                TStringStream stream;
                ::Save(&stream, pair.second.size() * sizeof(ui32));
                ::SaveArray(&stream, pair.second.data(), pair.second.size());
                RemapArrays.back().Load(TBlob::FromStream(stream));
                set.insert(std::make_pair(pair.second, idx));
                ++idx;
            }
            OutputToRemap[item.first].emplace_back(pair.first, set.at(pair.second));
        }
    }
    BuildOptimizedMappings();
}

void TGlobalSparsifier::BuildOptimizedMappings() {
    NeedLowercaseInput = false;
    NumTokenizerIds = 0;
    TokenizerIds.clear();
    TokenizerIds.reserve(Tokenizers.size());
    THashMap<TTokenizerUid, size_t> idByTokenizerUids;
    idByTokenizerUids.reserve(Tokenizers.size());
    for (const auto& tokenizer : Tokenizers) {
        NeedLowercaseInput |= tokenizer->NeedLowercaseInput();
        const size_t* id = idByTokenizerUids.FindPtr(tokenizer->GetUid());
        if (id) {
            TokenizerIds.push_back(*id);
        } else {
            idByTokenizerUids[tokenizer->GetUid()] = NumTokenizerIds;
            TokenizerIds.push_back(NumTokenizerIds);
            ++NumTokenizerIds;
        }
    }
    IndexByOutput.clear();
    IndexByOutput.reserve(OutputToRemap.size());
    RemapByOutputIndex.clear();
    RemapByOutputIndex.reserve(OutputToRemap.size());
    for (const auto& item : OutputToRemap) {
        IndexByOutput[item.first] = RemapByOutputIndex.size();
        TVector<std::pair<size_t, size_t>>& remap = RemapByOutputIndex.emplace_back();
        remap.reserve(item.second.size());
        for (const TTokenizerRemap& tr : item.second) {
            remap.emplace_back(idByTokenizerUids.at(tr.Uid), tr.Index);
        }
    }
}

void TGlobalSparsifier::DoRemap(const TPairOfIndexesAndValues& input,
    const TBlobArray<ui32>& remapVector, TSparseVector& output) const
{
    Y_ASSERT(input.first.size() == input.second.size());

    for (size_t i = 0; i < input.first.size(); ++i) {
        size_t ind = input.first[i];

        if (ind + 1 > remapVector.size()) {
            ind = remapVector.size() - 1;
        }
        if (remapVector[ind] != Max<ui32>()) {
            output.Indexes.push_back(remapVector[ind]);
            output.Values.push_back(input.second[i]);
        }
    }
}

TString TGlobalSparsifier::InternalsToString() const  {
    TStringStream s;
    ::Save(&s, Tokenizers.size());
    for (auto& tok : Tokenizers) {
        TString tokenizerName = tok->GetTypeName();
        ::Save(&s, tokenizerName.size());
        s << tokenizerName;
        tok->Save(&s);
    }

    ::Save(&s, RemapArrays.size());
    for (auto& remap : RemapArrays) {
        remap.Save(&s);
    }

    THashMap<TString, TString> hm;
    for (const auto& item : OutputToRemap) {
        TVector<TString> arr;
        for (const auto& i : item.second) {
            arr.push_back(i.ToString());
        }
        hm[item.first] = SaveToStroka(arr);
    }

    ::Save(&s, hm);

    return s.Str();
}

TString TGlobalSparsifier::TTokenizerRemap::ToString() const {
    TStringStream stream;
    ::Save(&stream, Uid.size());
    ::SaveArray(&stream, Uid.data(), Uid.size());
    ::Save(&stream, Index);

    return stream.Str();
}

void TGlobalSparsifier::TTokenizerRemap::FromString(const TString& str)  {
    TBlob blob = TBlob::FromString(str);
    Uid = TTokenizerUid(ReadString(blob));
    Index = ReadSize(blob);
}

void TGlobalSparsifier::Save(IOutputStream* s) const {
    const TString internals = InternalsToString();
    ::Save(s, internals.size());
    *s << internals;
}

size_t TGlobalSparsifier::Load(const TBlob& blob) {
    TBlob curBlob = blob;
    size_t totalLenth = ReadSize(curBlob);
    curBlob = curBlob.SubBlob(0, totalLenth);

    size_t numTokenizers = ReadSize(curBlob);
    for (size_t i = 0; i < numTokenizers; ++i) {
        TString tokenizerType = ReadString(curBlob);
        ITokenizer* tokenizer = CreateTokenizer(tokenizerType);
        size_t loaded = tokenizer->Load(curBlob);
        curBlob = curBlob.SubBlob(loaded, curBlob.Size());
        Tokenizers.push_back(tokenizer);
    }

    size_t numRemaps = ReadSize(curBlob);
    RemapArrays.resize(numRemaps);
    for (size_t i = 0; i < numRemaps; ++i) {
        size_t loaded = RemapArrays[i].Load(curBlob);
        curBlob = curBlob.SubBlob(loaded, curBlob.Size());
    }

    THashMap<TString, TString> hm = ReadFields32(curBlob);

    for (const auto& item : hm) {
        TVector<TString> v;
        LoadFromStroka(item.second, &v);
        auto& ref = OutputToRemap[item.first];
        for (const auto& str : v) {
            ref.emplace_back();
            ref.back().FromString(str);
        }
    }

    Y_ENSURE(curBlob.Empty(), "Incorrect GlobalSparsifier Load");

    BuildOptimizedMappings();

    return totalLenth + sizeof(size_t);
}

size_t TGlobalSparsifier::Load(const TBlob& blob, const TLoadParams& loadParams) {
    TBlob curBlob = blob;
    size_t totalLenth = ReadSize(curBlob);
    curBlob = curBlob.SubBlob(0, totalLenth);

    size_t numTokenizers = ReadSize(curBlob);
    for (size_t i = 0; i < numTokenizers; ++i) {
        TString tokenizerType = ReadString(curBlob);
        ITokenizer* tokenizer = CreateTokenizer(tokenizerType);
        size_t loaded = tokenizer->Load(curBlob, loadParams);
        curBlob = curBlob.SubBlob(loaded, curBlob.Size());
        Tokenizers.push_back(tokenizer);
    }

    size_t numRemaps = ReadSize(curBlob);
    RemapArrays.resize(numRemaps);
    for (size_t i = 0; i < numRemaps; ++i) {
        size_t loaded = RemapArrays[i].Load(curBlob);
        curBlob = curBlob.SubBlob(loaded, curBlob.Size());
    }

    THashMap<TString, TString> hm = ReadFields32(curBlob);

    for (const auto& item : hm) {
        TVector<TString> v;
        LoadFromStroka(item.second, &v);
        auto& ref = OutputToRemap[item.first];
        for (const auto& str : v) {
            ref.emplace_back();
            ref.back().FromString(str);
        }
    }

    Y_ENSURE(curBlob.Empty(), "Incorrect GlobalSparsifier Load");

    BuildOptimizedMappings();

    return totalLenth + sizeof(size_t);
}

THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>> TGlobalSparsifier::DoGetRemap() const {
    THashMap<TString, TVector<std::pair<TTokenizerUid, TVector<ui32>>>> ret;
    for (const auto& item : OutputToRemap) {
        for (const auto& pair : item.second) {
            ret[item.first].emplace_back(pair.Uid, TVector<ui32>(RemapArrays[pair.Index].begin(), RemapArrays[pair.Index].end()));
        }
    }
    return ret;
}

void TGlobalSparsifier::RenameVariable(const TString& name, const TString& newName) {
    auto it = OutputToRemap.find(name);
    if ((it != OutputToRemap.end()) && !name.equal(newName)) {
        OutputToRemap.emplace(newName, std::move(it->second));
        OutputToRemap.erase(it);
    }
}

void TGlobalSparsifier::MapOutputs(const TVector<TString>& outputs, TVector<size_t>& outputIndices) const {
    outputIndices.resize(outputs.size());
    for (size_t i = 0; i < outputs.size(); ++i) {
        outputIndices[i] = IndexByOutput.at(outputs[i]);
    }
}

void TGlobalSparsifier::ToSparse(
    const TString& text,
    const TVector<std::pair<size_t, TSparseVector&>>& result,
    TVector<TPairOfIndexesAndValues>& tokenizerResultsBuffer)
{
    TUtf16String wTextStr;
    try {
        wTextStr = UTF8ToWide(text);
    } catch (const yexception&) {
        wTextStr = ParseBrokenUTF8(text);
    }
    TUtf16String lowerWTextStr;
    if (NeedLowercaseInput) {
        lowerWTextStr = wTextStr;
        lowerWTextStr.to_lower();
    }
    tokenizerResultsBuffer.resize(NumTokenizerIds);
    for (TPairOfIndexesAndValues& tokenizerResult : tokenizerResultsBuffer){
        tokenizerResult.first.clear();
        tokenizerResult.first.reserve(Max(wTextStr.size(), lowerWTextStr.size()));
        tokenizerResult.second.clear();
        tokenizerResult.second.reserve(Max(wTextStr.size(), lowerWTextStr.size()));
    }
    for (size_t i = 0; i < Tokenizers.size(); ++i) {
         const auto& t = Tokenizers[i];
         const TUtf16String& input = t->NeedLowercaseInput() ? lowerWTextStr : wTextStr;
         auto& indexesAndValues = tokenizerResultsBuffer[TokenizerIds[i]];
         t->AddTokens(input, indexesAndValues.first, indexesAndValues.second);
    }
    for (const auto& p : result) {
        auto& sparseV = p.second;
        sparseV.Indexes.clear();
        sparseV.Values.clear();
        size_t totalSize = 0;
        for (const auto& tokInfo : RemapByOutputIndex[p.first]) {
            totalSize += tokenizerResultsBuffer[tokInfo.first].first.size();
        }
        sparseV.Indexes.reserve(totalSize);
        sparseV.Values.reserve(totalSize);
        for (const auto& tokInfo : RemapByOutputIndex[p.first]) {
            DoRemap(tokenizerResultsBuffer[tokInfo.first], RemapArrays[tokInfo.second], sparseV);
        }
    }
}

}

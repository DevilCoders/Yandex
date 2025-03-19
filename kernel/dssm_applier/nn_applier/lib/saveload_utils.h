#pragma once

#include <library/cpp/containers/comptrie/comptrie_builder.h>
#include <library/cpp/text_processing/dictionary/bpe_builder.h>

#include <util/ysaveload.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/stream/str.h>
#include <util/system/unaligned_mem.h>

namespace NNeuralNetApplier {

template <class T>
TString SaveToStroka(const T& item) {
    TStringStream ss;
    ::Save(&ss, item);
    return TString(ss.Data(), ss.Size());
}

template <class T>
void LoadFromStroka(const TString& s, T* item) {
    TStringStream ss;
    ss.Write(s.data(), s.size());
    ::Load(&ss, *item);
}

inline void SaveVectorStrok(IOutputStream* out, const TVector<TString>& vec) {
    size_t totalLength = 0;
    for (const auto& s : vec) {
        totalLength += sizeof(size_t) + s.size();
    }
    ::Save(out, totalLength);
    for (const auto& s : vec) {
        ::Save(out, s.size());
        *out << s;
    }
}

inline void SaveString64(IOutputStream* outStream, const TString& string) {
    ::Save(outStream, static_cast<ui64>(string.size()));
    ::SavePodArray(outStream, string.data(), string.size());
}

inline ui64 ReadSize(TBlob& blob) {
    const ui64 result = ReadUnaligned<ui64>(blob.Begin());
    blob = blob.SubBlob(sizeof(ui64), blob.Size());
    return result;
}

inline ui32 ReadSize32(TBlob& blob) {
    const ui32 result = ReadUnaligned<ui32>(blob.Begin());
    blob = blob.SubBlob(sizeof(ui32), blob.Size());
    return result;
}

inline TString ReadString(TBlob& blob) {
    size_t stringSize = ReadSize(blob);
    TString result(blob.AsCharPtr(), stringSize);
    blob = blob.SubBlob(stringSize, blob.Size());
    return result;
}

inline TString ReadString32(TBlob& blob) {
    size_t stringSize = ReadSize32(blob);
    TString result(blob.AsCharPtr(), stringSize);
    blob = blob.SubBlob(stringSize, blob.Size());
    return result;
}

inline void SaveFields64(IOutputStream* os, const THashMap<TString, TString>& fields) {
    TString serialized = SaveToStroka(fields);
    SaveString64(os, serialized);
}

inline THashMap<TString, TString> ReadFields(TBlob& blob) {
    TString fieldsString = ReadString(blob);
    THashMap<TString, TString> fields;
    LoadFromStroka(fieldsString, &fields);
    return fields;
}

inline THashMap<TString, TString> ReadFields32(TBlob& blob) {
    THashMap<TString, TString> result;
    size_t numFields = ReadSize32(blob);
    for (size_t i = 0; i < numFields; ++i) {
        TString k = ReadString32(blob);
        TString v = ReadString32(blob);
        result[k] = v;
    }

    return result;
}

inline TBlob ReadBlob(TBlob& blob) {
    size_t blobSize = ReadSize(blob);
    TBlob result = blob.SubBlob(0, blobSize);
    blob = blob.SubBlob(blobSize, blob.Size());
    return result;
}

using TTermToIndex = THashMap<TUtf16String, size_t>;

inline TString HashMapToTrieString(const TTermToIndex& mapping) {
    TVector<std::pair<TUtf16String, size_t>> pairs(mapping.begin(), mapping.end());
    Sort(pairs.begin(), pairs.end());
    TCompactTrieBuilder<TUtf16String::char_type, size_t> builder;
    for (const auto& p : pairs) {
        builder.Add(p.first.data(), p.first.size(), p.second);
    }
    TStringStream out;
    builder.Save(out);
    return TString(out.Data(), out.Size());
}

inline TString TrieToString(const TCompactTrie<TUtf16String::char_type, size_t>& trie) {
   return TString(trie.Data().AsCharPtr(), trie.Data().Size());
}

}  // namespace NNeuralNetApplier

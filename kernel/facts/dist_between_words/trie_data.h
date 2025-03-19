#pragma once

#include <library/cpp/containers/comptrie/comptrie.h>

#include <util/generic/string.h>

namespace NDistBetweenWords {
    TUtf16String BuildKey(const TString& word1, const TString& word2);
    TUtf16String BuildKey(const TString& word);
    TUtf16String BuildKey(const TUtf16String& word1, const TUtf16String& word2);
    TUtf16String BuildKey(const TUtf16String& word);

    struct TTrieData {

        TTrieData() = default;

        TTrieData(float host5,
                  float host10,
                  float url5,
                  float url10) noexcept:
                Host5(host5),
                Host10(host10),
                Url5(url5),
                Url10(url10)
        {}

        using TGetter = std::function<float (const TTrieData&)>;

        float Host5;
        float Host10;
        float Url5;
        float Url10;
    };

    IOutputStream& operator << (IOutputStream& stream, const TTrieData& data);

    using TWordDistTrieBuilder = TCompactTrieBuilder<wchar16, TTrieData, TAsIsPacker<TTrieData>>;
    using TWordDistTrie = TCompactTrie<wchar16, TTrieData, TAsIsPacker<TTrieData>>;
    using TWordDistTriePtr = TAtomicSharedPtr<TWordDistTrie>;
}

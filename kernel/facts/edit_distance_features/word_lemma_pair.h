#pragma once

#include <kernel/lemmer/core/lemmer.h>

#include <util/generic/hash.h>
#include <util/generic/string.h>

namespace NEditDistanceFeatures {

    class TWordLemmaPair {
    public:
        using TLemmaArrayMap = THashMap<TUtf16String, TWLemmaArray>;

    public:
        // FACTS-1770: the only supported languages yet are Russian and English
        static TWordLemmaPair GetCachedWordLemmaPair(const TUtf16String& word, TLemmaArrayMap& lemmaCache, const TLangMask& langs);

    public:
        TWordLemmaPair(const TUtf16String& word, TWLemmaArray& lemmas);

        bool operator==(const TWordLemmaPair& rhs) const;
        bool operator!=(const TWordLemmaPair& rhs) const;
        bool operator<(const TWordLemmaPair& rhs) const;

        bool Like(const TWordLemmaPair& rhs) const;

        const TUtf16String& GetWord() const;
        const TWLemmaArray& GetLemmas() const;

    private:
        TUtf16String Word;
        TWLemmaArray* Lemmas;
    };

}

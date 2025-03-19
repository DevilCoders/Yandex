#pragma once

#include <library/cpp/token/formtype.h>
#include <library/cpp/token/token_structure.h>
#include <kernel/lemmer/core/langcontext.h>
#include <kernel/lemmer/core/wordinstance.h>
#include <util/generic/string.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/system/rwlock.h>

struct TRequesterData;
class TWordFilter;
class TFormDecimator;

namespace NLemmerCache {
    struct TKey {
        TUtf16String Word;
        TCharSpan Span;
        TFormType Type;

        // Necessary fields from TLanguageContext.
        TLangMask LangMask;
        bool FilterForms;

        TKey()
            : Type(fGeneral)
            , FilterForms(true)
        {}

        TKey(const TUtf16String& word, const TCharSpan& span, TFormType type, const TLanguageContext& context)
            : Word(word)
            , Span(span)
            , Type(type)
            , LangMask(context.GetLangMask())
            , FilterForms(context.FilterForms)
        {}

        bool operator==(const TKey& other) const;
        bool operator!=(const TKey& other) const {
            return !(*this == other);
        }

        size_t Hash() const;
        TString ToJson() const;
        void FromJson(const TString& json);
    };
}

template<>
struct THash<NLemmerCache::TKey> {
    size_t operator()(const NLemmerCache::TKey& k) const {
        return k.Hash();
    }
};

//---------------------------------------------------------------------------

class TLemmerCache {
public:
    typedef NLemmerCache::TKey TKey;

    TLemmerCache()
        : StopWords(nullptr)
        , Decimator(nullptr)
    {}

    bool Find(const TKey& key, TWordInstance& result) const;
    void Set(const TKey& key, TWordInstance instance);
    void SetExternals(const TRequesterData& externals);
    bool HasCorrectExternals(const TLanguageContext& context) const;
    void Clear();

private:
    typedef THashMap<TKey, TWordInstance> TBody;

    TBody Body;
    mutable TRWMutex Mutex;
    // Externals.
    const TWordFilter* StopWords;
    const TFormDecimator* Decimator;
};

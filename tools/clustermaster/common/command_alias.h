#pragma once

#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NCA {

struct TNoAlias: yexception {};

struct TCommandAlias {
    typedef TStringBuf TAlias;
    typedef ui32 TFlags;

    const TAlias Alias;
    const TFlags Flags;

    const TStringBuf Description;

    TCommandAlias(const TStringBuf alias, ui32 flags, const TStringBuf description)
        : Alias(alias)
        , Flags(flags)
        , Description(description)
    {
    }
};

const TCommandAlias* CommandAliasesBegin() noexcept;
const TCommandAlias* CommandAliasesEnd() noexcept;

bool IsCommandAlias(TStringBuf alias) noexcept;
TStringBuf GetFirstAlias(ui32 flags);
ui32 GetFlags(TStringBuf alias);
TStringBuf GetDescription(TStringBuf alias);

}

namespace NStaticStringSetPrivate {

struct TMakeStringBuf {
    inline const TStringBuf operator()(const char* from) const noexcept {
        return TStringBuf(from);
    }
};

}

struct TStaticStringSet
    : private THashTable<const char*, TStringBuf, THash<TStringBuf>, NStaticStringSetPrivate::TMakeStringBuf, TEqualTo<TStringBuf>, std::allocator<const char*>>
{
    typedef THashTable<value_type, key_type, hasher, NStaticStringSetPrivate::TMakeStringBuf, key_equal, std::allocator<value_type>> TBase;

    template <size_t SIZE>
    inline TStaticStringSet(const char * const (&table)[SIZE])
        : TBase(SIZE, hasher(), key_equal())
    {
        insert_equal(reinterpret_cast<const char* const*>(table), reinterpret_cast<const char* const*>(table) + SIZE);
    }

    template <class TExtractString, class T, size_t SIZE>
    inline TStaticStringSet(const TExtractString&, const T (&table)[SIZE])
        : TBase(SIZE, hasher(), key_equal())
    {
        TExtractString extractor;

        for (const T* i = table; i != table + SIZE; ++i)
            insert_equal(extractor(*i));
    }

    inline bool operator()(TStringBuf what) const noexcept {
        return find(what) != end();
    }
};

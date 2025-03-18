#include "command_alias.h"

#include "messages.h"

#include <util/generic/hash.h>

namespace NCA{
namespace {

const TCommandAlias CommandAliases[] = {
    TCommandAlias("run",                    TCommandMessage::CF_RUN, "Run"),
    TCommandAlias("run-path",               TCommandMessage::CF_RUN | TCommandMessage::CF_RECURSIVE_UP, "Run whole path"),
    TCommandAlias("run-following",          TCommandMessage::CF_RUN | TCommandMessage::CF_RECURSIVE_DOWN, "Run with followers"),

    TCommandAlias("retry-run",              TCommandMessage::CF_RUN | TCommandMessage::CF_RETRY, "Retry run"),
    TCommandAlias("retry-run-path",         TCommandMessage::CF_RUN | TCommandMessage::CF_RETRY | TCommandMessage::CF_RECURSIVE_UP, "Retry run whole path"),
    TCommandAlias("retry-run-following",    TCommandMessage::CF_RUN | TCommandMessage::CF_RETRY | TCommandMessage::CF_RECURSIVE_DOWN, "Retry run with followers"),

    TCommandAlias("forced-run",             TCommandMessage::CF_RUN | TCommandMessage::CF_FORCE_RUN, "Forced run"),
    TCommandAlias("forced-ready",           TCommandMessage::CF_RUN | TCommandMessage::CF_FORCE_READY, "Forced ready"),

    TCommandAlias("invalidate",             TCommandMessage::CF_INVALIDATE, "Invalidate"),
    TCommandAlias("invalidate-path",        TCommandMessage::CF_INVALIDATE | TCommandMessage::CF_RECURSIVE_UP, "Invalidate whole path"),
    TCommandAlias("invalidate-following",   TCommandMessage::CF_INVALIDATE | TCommandMessage::CF_RECURSIVE_DOWN, "Invalidate with followers"),

    TCommandAlias("cancel",                 TCommandMessage::CF_CANCEL, "Cancel"),
    TCommandAlias("cancel-path",            TCommandMessage::CF_CANCEL | TCommandMessage::CF_RECURSIVE_UP, "Cancel whole path"),
    TCommandAlias("cancel-following",       TCommandMessage::CF_CANCEL | TCommandMessage::CF_RECURSIVE_DOWN, "Cancel with followers"),

    TCommandAlias("reset",                  TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE, "Reset"),
    TCommandAlias("reset-path",             TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE | TCommandMessage::CF_RECURSIVE_UP, "Reset whole path"),
    TCommandAlias("reset-following",        TCommandMessage::CF_CANCEL | TCommandMessage::CF_INVALIDATE | TCommandMessage::CF_RECURSIVE_DOWN, "Reset with followers"),

    TCommandAlias("mark-success",           TCommandMessage::CF_MARK_SUCCESS, "Mark as successful"),
    TCommandAlias("reset-stat",             TCommandMessage::CF_RESET_STAT, "Reset resources usage statistics"),
};

struct TExtractAlias {
    inline const TCommandAlias::TAlias& operator()(const TCommandAlias* from) const noexcept {
        return from->Alias;
    }
};

struct TAliasHashtable
    : THashTable<const TCommandAlias*, TCommandAlias::TAlias, THash<TCommandAlias::TAlias>, TExtractAlias, TEqualTo<TCommandAlias::TAlias>, std::allocator<const TCommandAlias*>>
{
    typedef THashTable<value_type, key_type, hasher, TExtractAlias, key_equal, std::allocator<value_type>> TBase;

    template <size_t SIZE>
    TAliasHashtable(const TCommandAlias (&table)[SIZE])
        : TBase(SIZE, hasher(), key_equal())
    {
        for (const TCommandAlias* i = reinterpret_cast<const TCommandAlias* const>(table); i != reinterpret_cast<const TCommandAlias* const>(table) + SIZE; ++i)
            TBase::insert_unique(i);
    }
} const AliasHashtable(CommandAliases);

struct TExtractFlags {
    inline const TCommandAlias::TFlags& operator()(const TCommandAlias* from) const noexcept {
        return from->Flags;
    }
};

struct TFlagsHashtable
    : THashTable<const TCommandAlias*, TCommandAlias::TFlags, THash<TCommandAlias::TFlags>, TExtractFlags, TEqualTo<TCommandAlias::TFlags>, std::allocator<const TCommandAlias*>>
{
    typedef THashTable<value_type, key_type, hasher, TExtractFlags, key_equal, std::allocator<value_type>> TBase;

    template <size_t SIZE>
    TFlagsHashtable(const TCommandAlias (&table)[SIZE])
        : TBase(SIZE, hasher(), key_equal())
    {
        for (const TCommandAlias* i = reinterpret_cast<const TCommandAlias* const>(table); i != reinterpret_cast<const TCommandAlias* const>(table) + SIZE; ++i)
            TBase::insert_equal(i);
    }
} const FlagsHashtable(CommandAliases);

} // NCA
} // anonymous namespace

namespace NCA {

const TCommandAlias* CommandAliasesBegin() noexcept {
    return CommandAliases;
}

const TCommandAlias* CommandAliasesEnd() noexcept {
    return CommandAliases + Y_ARRAY_SIZE(CommandAliases);
}

bool IsCommandAlias(TStringBuf alias) noexcept {
    return AliasHashtable.find(alias) != AliasHashtable.end();
}

TStringBuf GetFirstAlias(ui32 flags) {
    const TFlagsHashtable::const_iterator i = FlagsHashtable.find(flags);

    if (i == FlagsHashtable.end())
        throw TNoAlias() << "there is no alias for flags "sv << flags;

    return TExtractAlias()(*i);
}

ui32 GetFlags(TStringBuf alias) {
    const TAliasHashtable::const_iterator i = AliasHashtable.find(alias);

    if (i == AliasHashtable.end())
        throw TNoAlias() << "there is no alias \""sv << alias << '\"';

    return TExtractFlags()(*i);
}

TStringBuf GetDescription(TStringBuf alias) {
    const TAliasHashtable::const_iterator i = AliasHashtable.find(alias);

    if (i == AliasHashtable.end())
        throw TNoAlias() << "there is no alias \""sv << alias << '\"';

    return (*i)->Description;
}

}

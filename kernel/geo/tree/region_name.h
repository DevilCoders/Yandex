#pragma once

#include <util/generic/string.h>
#include <util/generic/hash.h>

#include <library/cpp/langmask/langmask.h>

#include <kernel/search_types/search_types.h>

#include <array>

class TRegionNameData {
public:
    TRegionNameData() = default;

    struct TLrName {
        enum {
            Russian,
            Ukrainian,
            Kazakh,
            English,
            Belarussian,
            Tatar,
            Turkish,
            NameSize
        };
        std::array<TString, NameSize> Name;
    };

    const TString& GetLrName(const TCateg lr, const TCateg countryId, const TLangMask& langMask) const;
    void ReadLrToName(const TString& fileName);

private:
    using TLrToName = THashMap<TCateg, TLrName>;

    TLrToName LrToName;
};

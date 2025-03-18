#pragma once

#include "req_types.h"
#include "request_params.h"

#include <library/cpp/archive/yarchive.h>

#include <util/digest/numeric.h>
#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>

#include <array>
#include <functional>


namespace NAntiRobot {


struct TResourceKey {
public:
    ELanguage Lang;
    EHostType Service;

public:
    explicit TResourceKey(
        ELanguage lang = LANG_UNK,
        EHostType service = EHostType::Count
    )
        : Lang(lang)
        , Service(service)
    {}

    static TMaybe<TResourceKey> Parse(TStringBuf keyStr);

    TResourceKey(const TRequest& req)
        : Lang(req.Lang)
        , Service(req.HostType)
    {}

    bool operator==(const TResourceKey& that) const {
        return Lang == that.Lang && Service == that.Service;
    }

    bool operator!=(const TResourceKey& that) const {
        return !(*this == that);
    }
};


template <typename T>
class TResourceSelector {
public:
    using TResource = T;

public:
    template <typename F>
    TResourceSelector(const TArchiveReader& reader, TStringBuf prefix, F makeResource) {
        for (size_t i = 0; i < reader.Count(); i++) {
            const TString arcKey = reader.KeyByIndex(i);
            const TStringBuf arcKeyRef = arcKey;

            if (!arcKeyRef.StartsWith(prefix)) {
                continue;
            }

            const auto keyStr = arcKeyRef.substr(prefix.size());
            const auto key = TResourceKey::Parse(keyStr);

            Y_ENSURE(key, "Bad archive key: " << arcKey);

            ResourceByKey.emplace(*key, makeResource(
                keyStr,
                reader.ObjectByKey(arcKey)->ReadAll()
            ));
        }

        for (const auto lang : {LANG_RUS, LANG_ENG}) {
            Y_ENSURE(
                ResourceByKey.contains(TResourceKey(lang)),
                "Missing fallback resource"
            );
        }
    }

    const TResource& Select(const TRequest& req) const {
        const TResourceKey key(req);
        const auto fallbackLang = GetFallbackLang(key.Lang);

        const std::array<TResourceKey, 5> keys = {
            key,
            TResourceKey(fallbackLang, key.Service),
            TResourceKey(LANG_RUS, key.Service),
            TResourceKey(key.Lang),
            TResourceKey(fallbackLang)
        };

        for (const auto& key : keys) {
            if (const auto resource = ResourceByKey.FindPtr(key)) {
                return *resource;
            }
        }

        ythrow yexception() <<
            "Failed to find resource for language '" << key.Lang << "' and "
            "service '" << key.Service << "'";
    }

private:
    static constexpr ELanguage GetFallbackLang(ELanguage lang)  {
        return Find(LanguagesWithRussianFallback, lang) != LanguagesWithRussianFallback.end() ?
            LANG_RUS : LANG_ENG;
    }

private:
    THashMap<TResourceKey, TResource> ResourceByKey;

    static constexpr std::array<ELanguage, 5> LanguagesWithRussianFallback = {
        LANG_UKR,
        LANG_BEL,
        LANG_KAZ,
        LANG_UZB,
        LANG_TAT
    };
};


}


template <>
struct THash<NAntiRobot::TResourceKey> {
    size_t operator()(const NAntiRobot::TResourceKey& key) const {
        return IntHash(static_cast<size_t>(key.Lang) * NAntiRobot::EHostType::Count + key.Service);
    }
};

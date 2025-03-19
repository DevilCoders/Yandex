#include "saas_service_opts.h"

#include <kernel/querydata/saas/idl/saas_service_options.sc.h>
#include <kernel/querydata/saas/qd_saas_request.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>

#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>

#include <util/string/builder.h>
#include <util/string/join.h>

using namespace NQueryDataSaaS;

namespace NQuerySearch {
    namespace {
        template<typename T>
        bool IsPrefix(const TVector<T>& keyType, const TVector<T>& prefix) {
            if (prefix.size() > keyType.size()) {
                return false;
            }
            for (size_t i = 0, imax = prefix.size(); i < imax; ++i) {
                if (prefix[i] != keyType[i]) {
                    return false;
                }
            }
            return true;
        }

        using TKeyTypeMap = TMap<NQueryDataSaaS::TSaaSKeyType, TSaaSKeyTypeOpts>;
        void JoinPrefixGroup(TKeyTypeMap& keyTypes, TKeyTypeMap::iterator from, TKeyTypeMap::iterator to) {
            auto& prefixSet = to->second.PrefixSet;
            while (from != to) {
                if (from->second.Disable || from->second.LastRealmUnique) {
                    ++from;
                } else {
                    if (from->first.size() > 0) {
                        prefixSet.set(from->first.size() - 1, true);
                    }
                    keyTypes.erase(from++);
                }
            }
        }

        const TSaaSKeyType UrlKey{SST_IN_KEY_URL};
        const TSaaSKeyType UrlMaskKey{SST_IN_KEY_URL_MASK};
    }

    bool TSaaSServiceOpts::IsDisabled() const {
        return !AnyOf(KeyTypes, [](const auto& kt) {
            return kt.first && !kt.second.Disable;
        });
    }

    TVector<TSaaSKeyType> TSaaSServiceOpts::GetKeyTypes() const {
        TVector<TSaaSKeyType> res;
        res.reserve(KeyTypes.size());
        for (const auto& item : KeyTypes) {
            if (!item.second.Disable && item.first) {
                res.emplace_back(item.first);
            }
        }
        return res;
    }

    void TSaaSServiceOpts::InitFromJsonThrow(TStringBuf json) {
        NSc::TValue sc = NSc::TValue::FromJsonThrow(json);
        InitFromSchemeThrow(sc);
    }

    void TSaaSServiceOpts::InitFromSchemeThrow(const NSc::TValue& val) {
        TSaaSServiceOptionsConst<TSchemeTraits> opts{&val};

        TVector<TString> errors;
        auto onError = [&errors](const TString& path, const TString& error) {
            errors.emplace_back(TStringBuilder() << "invalid options at " << path.Quote() << ": " << error);
        };

        Y_ENSURE(opts.Validate(TString(), false, onError), "validation failed (" << JoinSeq("; ", errors));

        MaxKeysInRequest = opts.MaxKeysInRequest().Get();
        MaxRequests = opts.MaxRequests().Get();
        DefaultKps = opts.DefaultKps().Get();
        SaaSType = FromString(opts.SaaSType().Get());
        OptimizeKeyTypes = opts.OptimizeKeyTypes().Get();

        KeyTypesOriginal.clear();
        KeyTypes.clear();
        for (const auto& keyType : opts.KeyTypes()) {
            auto kt = FromString<TSaaSKeyType>(keyType.Key());
            auto& keyTypeOpt = KeyTypesOriginal[kt];
            keyTypeOpt.Disable = keyType.Value().Disable().Get();
            keyTypeOpt.LastRealmUnique = keyType.Value().LastUnique().Get();
            if (!keyTypeOpt.Disable) {
                KeyTypes[kt] = keyTypeOpt;
            }
        }

        if (OptimizeKeyTypes) {
            auto it = KeyTypes.begin();
            auto previousIt = it;
            auto groupIt = it;
            auto itEnd = KeyTypes.end();
            for (; it != itEnd; previousIt = it++) {
                if (it != groupIt) {
                    if (!IsPrefix(it->first, previousIt->first)) {
                        if (previousIt != groupIt) {
                            JoinPrefixGroup(KeyTypes, groupIt, previousIt);
                        }
                        groupIt = it;
                    }
                }
            }
            if (previousIt != groupIt) {
                JoinPrefixGroup(KeyTypes, groupIt, previousIt);
            }
        }

        auto urlMaskIt = KeyTypes.find(UrlMaskKey);
        if (urlMaskIt != KeyTypes.end() && !urlMaskIt->second.Disable) {
            bool foundUrl = false;
            auto urlIt = KeyTypes.lower_bound(UrlKey);
            for (; urlIt != KeyTypes.end() && urlIt->first.front() == SST_IN_KEY_URL; ++urlIt) {
                if (!urlIt->second.Disable) {
                    foundUrl = true;
                    urlIt->second.UrlMaskSearch = true;
                }
            }
            if (!foundUrl) {
                auto& urlKey = KeyTypes[UrlKey];
                urlKey.Disable = false;
                urlKey.UrlMaskSearch = true;
                urlKey.NormalSearch = false;
            }
            KeyTypes.erase(urlMaskIt);
        }
    }

    NSc::TValue TSaaSServiceOpts::AsScheme() const {
        NSc::TValue val;
        {
            TSaaSServiceOptions<TSchemeTraits> opts{&val};
            opts.MaxKeysInRequest().Set(MaxKeysInRequest);
            opts.MaxRequests().Set(MaxRequests);
            opts.DefaultKps().Set(DefaultKps);
            opts.SaaSType().Set(ToString(SaaSType));
            opts.OptimizeKeyTypes().Set(OptimizeKeyTypes);

            for (const auto& keyType : KeyTypesOriginal) {
                auto keyTypeOpt = opts.KeyTypes()[ToString(keyType.first)];
                keyTypeOpt.Disable().Set(keyType.second.Disable);
                keyTypeOpt.LastUnique().Set(keyType.second.LastRealmUnique);
            }
        }
        return val;
    }

    TString TSaaSServiceOpts::AsJson() const {
        return AsScheme().ToJson();
    }
}

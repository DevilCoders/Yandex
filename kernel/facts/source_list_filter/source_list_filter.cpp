#include "source_list_filter.h"

#include <kernel/facts/source_list_filter/source_list_filter_config.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <search/session/searcherprops.h>

#include <util/generic/algorithm.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>


namespace NFacts {

TSourceListFilter::TSourceListFilter(TStringBuf configJson, EFilterMode filterMode)
    : TSourceListFilter(TVector<TStringBuf>{configJson}, filterMode)
{}


TSourceListFilter::TSourceListFilter(const TVector<TStringBuf>& configJsons, EFilterMode filterMode) {
    ConfigStorages.resize(configJsons.size());  // eliminate further resizes and references-to-items invalidation

    for (size_t i = 0; i < ConfigStorages.size(); ++i) {
        ConfigStorages[i] = NSc::TValue::FromJsonThrow(configJsons[i], NSc::TJsonOpts(
                NSc::TJsonOpts::JO_PARSER_STRICT_JSON |
                NSc::TJsonOpts::JO_PARSER_STRICT_UTF8 |
                NSc::TJsonOpts::JO_PARSER_DISALLOW_COMMENTS |
                NSc::TJsonOpts::JO_PARSER_DISALLOW_DUPLICATE_KEYS));

        TSourceListFilterConfigConst<TSchemeTraits> parsedConfig(&ConfigStorages[i]);
        parsedConfig.Validate(/*path*/ "", /*strict*/ false,
                /*onError*/ [](const TString& path, const TString& message, const NDomSchemeRuntime::TValidateInfo& status) -> void {
                    ythrow yexception() << ToString(status.Severity) << ": path '" << path << "' " << message;
                });

        {
            const auto& sources = parsedConfig.Sources();
            Config.Sources.reserve(Config.Sources.size() + sources.Size());
            Config.Sources.insert(sources.begin(), sources.end());
        }
        {
            const auto& serpTypes = parsedConfig.SerpTypes();
            Config.SerpTypes.reserve(Config.SerpTypes.size() + serpTypes.Size());
            Config.SerpTypes.insert(serpTypes.begin(), serpTypes.end());
        }
        {
            const auto& factTypes = parsedConfig.FactTypes();
            Config.FactTypes.reserve(Config.FactTypes.size() + factTypes.Size());
            Config.FactTypes.insert(factTypes.begin(), factTypes.end());
        }
        {
            const auto& hostnames = parsedConfig.Hostnames();
            Config.Hostnames.reserve(Config.Hostnames.size() + hostnames.Size());
            Config.Hostnames.insert(hostnames.begin(), hostnames.end());
        }
    }

    Config.FilterMode = filterMode;
}


bool TSourceListFilter::IsSourceListed(TStringBuf source, TStringBuf serpType, TStringBuf factType, const TVector<TString>& hostnames) const {
    const TString sourceAsValidKey = TSearcherProps::ConvertToValidKey(source, '_');
    const TString serpTypeAsValidKey = TSearcherProps::ConvertToValidKey(serpType, '_');
    const TString factTypeAsValidKey = TSearcherProps::ConvertToValidKey(factType, '_');

    if (Config.Sources.contains(sourceAsValidKey) || Config.SerpTypes.contains(serpTypeAsValidKey) || Config.FactTypes.contains(factTypeAsValidKey)) {
        return true;
    }

    switch (Config.FilterMode) {
    case EFilterMode::Whitelist:
        if (!hostnames.empty() && AllOf(hostnames.begin(), hostnames.end(), [this](const TString& hostname) { return Config.Hostnames.contains(hostname); })) {
            return true;
        }
        break;
    case EFilterMode::Blacklist:
        if (AnyOf(hostnames.begin(), hostnames.end(), [this](const TString& hostname) { return Config.Hostnames.contains(hostname); })) {
            return true;
        }
        break;
    }

    return false;
}

}  // namespace NFacts

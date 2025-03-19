#include "compound_filter.h"

#include <kernel/facts/compound_filter/compound_filter_config.sc.h>

#include <library/cpp/scheme/domscheme_traits.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/string/cast.h>
#include <util/string/vector.h>


namespace NFacts {

TCompoundFilter::TCompoundFilter(TStringBuf configJson)
    : ConfigStorage(NSc::TValue::FromJsonThrow(configJson))
{
    TCompoundFilterConfigConst<TSchemeTraits> config(&ConfigStorage);
    config.Validate(/*path*/ "", /*strict*/ false,
        /*onError*/ [](const TString& path, const TString& message, const NDomSchemeRuntime::TValidateInfo& status) -> void {
            ythrow yexception() << ToString(status.Severity) << ": path \"" << path << "\" " << message;
        });

    const auto& blacklist = config.Blacklist();
    {
        {
            const auto& types = blacklist.Types();
            Config.Blacklist.Types.reserve(types.Size());
            for (TStringBuf type : types) {
                Config.Blacklist.Types.emplace(type);
            }
        }
        {
            const auto& sources = blacklist.Sources();
            Config.Blacklist.Sources.reserve(sources.Size());
            for (TStringBuf source : sources) {
                Config.Blacklist.Sources.emplace(source);
            }
        }
        {
            const auto& substrings = blacklist.Substrings();
            Config.Blacklist.Substrings.reserve(substrings.Size());
            for (TStringBuf substring : substrings) {
                Config.Blacklist.Substrings.emplace_back(substring);
            }
        }
    }

    const auto& whitelist = config.Whitelist();
    if (!whitelist.IsNull()) {
        {
            const auto& sources = whitelist.Sources();
            Config.Whitelist.Sources.reserve(sources.Size());
            for (TStringBuf source : sources) {
                Config.Whitelist.Sources.emplace(source);
            }
        }
        {
            const auto& hostnames = whitelist.Hostnames();
            Config.Whitelist.Hostnames.reserve(hostnames.Size());
            for (TStringBuf hostname : hostnames) {
                Config.Whitelist.Hostnames.emplace(hostname);
            }
        }
        {
            const auto& sourceAndHostnames = whitelist.SourceAndHostnames();
            Config.Whitelist.SourceAndHostnames.reserve(sourceAndHostnames.Size());
            for (const auto& sourceAndHostname : sourceAndHostnames) {
                Config.Whitelist.SourceAndHostnames.emplace(sourceAndHostname.Source(), sourceAndHostname.Hostname());
            }
        }
    }
}

TStringBuf TCompoundFilter::Filter(TStringBuf type, TStringBuf source, const TVector<TString>& hostnames, TStringBuf text) const {
    if (Config.Blacklist.Types.empty() && Config.Blacklist.Sources.empty()) {
        // If both 'blacklist/types' and 'blacklist/sources' are empty or omitted, then apply the blacklist regardless of the fact's type and source.
    } else {
        if (Config.Blacklist.Types.contains(type) || Config.Blacklist.Sources.contains(source)) {
            // Proceed with further checks...
        } else {
            return {};
        }
    }

    if (Config.Whitelist.Sources.contains(source) ||
        (!hostnames.empty() && AllOf(hostnames.begin(), hostnames.end(), [&source, this] (const TString& hostname) {
            return Config.Whitelist.Hostnames.contains(hostname) || Config.Whitelist.SourceAndHostnames.contains(std::make_tuple(source, hostname)); })))
    {
        return {};
    }

    for (TStringBuf substring : Config.Blacklist.Substrings) {
        if (text.find(substring) != TStringBuf::npos) {
            return substring;  // banned
        }
    }

    return {};  // passed
}

}  // namespace NFacts

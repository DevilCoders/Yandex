#pragma once

#include <kernel/common_server/common/scheme.h>
#include <kernel/common_server/abstract/frontend.h>

#include <library/cpp/regex/pcre/regexp.h>


class THostFilter {
private:
    CSA_READONLY_DEF(TString, HostPattern);
    CSA_READONLY_DEF(TString, FQDNHostPattern);
    CSA_DEFAULT(THostFilter, TSet<TString>, CTypes);
    CSA_READONLY_DEF(TRegExMatch, HostPatternRegex);
    CSA_READONLY_DEF(TRegExMatch, FQDNHostPatternRegex);
public:

    TString GetTitle() const;

    THostFilter& SetHostPattern(const TString& hostPattern);

    THostFilter& SetFQDNHostPattern(const TString& hostPattern);

    bool CheckHost(const IBaseServer& server) const;

    NJson::TJsonValue SerializeToJson() const;

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonInfo);

    static NFrontend::TScheme GetScheme(const IBaseServer& server, const TString& defaults);
};

#include "host_filter.h"

#include <util/system/hostname.h>

TString THostFilter::GetTitle() const {
    if (CTypes.size()) {
        return JoinSeq(", ", CTypes);
    } else if (HostPattern) {
        return HostPattern;
    } else if (FQDNHostPattern) {
        return FQDNHostPattern;
    } else {
        return "no_hosts_filter";
    }
}

THostFilter& THostFilter::SetHostPattern(const TString& hostPattern) {
    HostPattern = hostPattern;
    if (!!HostPattern) {
        HostPatternRegex = TRegExMatch(HostPattern);
    }
    return *this;
}

THostFilter& THostFilter::SetFQDNHostPattern(const TString& hostPattern) {
    FQDNHostPattern = hostPattern;
    if (!!FQDNHostPattern) {
        FQDNHostPatternRegex = TRegExMatch(FQDNHostPattern);
    }
    return *this;
}

bool THostFilter::CheckHost(const IBaseServer& server) const {
    if (!CTypes.empty() && !CTypes.contains(server.GetCType())) {
        return false;
    }
    try {
        if (!!HostPattern) {
            const TString hostName = GetHostName();
            if (!HostPatternRegex.Match(hostName.c_str())) {
                return false;
            }
        }
        if (!!FQDNHostPattern) {
            const TString fqdnHostName = GetFQDNHostName();
            if (!FQDNHostPatternRegex.Match(fqdnHostName.c_str())) {
                return false;
            }
        }
    } catch (...) {
        ERROR_LOG << CurrentExceptionMessage() << Endl;
        return false;
    }
    return true;
}


NJson::TJsonValue THostFilter::SerializeToJson() const {
    NJson::TJsonValue result = NJson::JSON_MAP;
    JWRITE(result, "host_pattern", HostPattern);
    JWRITE(result, "fqdn_host_pattern", FQDNHostPattern);
    JWRITE(result, "ctype", JoinSeq(",", CTypes));
    return result;
}


bool THostFilter::DeserializeFromJson(const NJson::TJsonValue& jsonInfo) {
    JREAD_STRING_OPT(jsonInfo, "host_pattern", HostPattern);
    JREAD_STRING_OPT(jsonInfo, "fqdn_host_pattern", FQDNHostPattern);
    if (!TJsonProcessor::ReadContainer(jsonInfo, "ctype", CTypes, false, ",")) {
        return false;
    }
    SetHostPattern(HostPattern);
    SetFQDNHostPattern(FQDNHostPattern);
    return true;
}


NFrontend::TScheme THostFilter::GetScheme(const IBaseServer& /*server*/, const TString& defaults) {
    NFrontend::TScheme result;
    result.Add<TFSString>("host_pattern", "Шаблон для имени хоста").SetRequired(false);
    result.Add<TFSString>("fqdn_host_pattern", "Шаблон для fqdn имени хоста").SetRequired(false);
    result.Add<TFSString>("ctype", "CTypes для запуска").SetDefault(defaults).SetRequired(false);
    return result;
}

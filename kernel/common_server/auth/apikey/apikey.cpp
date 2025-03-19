#include "apikey.h"
#include <library/cpp/digest/md5/md5.h>


TApikeyAuthConfig::TFactory::TRegistrator<TApikeyAuthConfig> TApikeyAuthConfig::Registrator("apikey");

TSet<TString> TApikeyAuthConfig::RequiredModules = { "EventsLimiter" };

const TString TApikeyAuthInfo::Name = "apikey";


IAuthInfo::TPtr TApikeyAuthModule::DoRestoreAuthInfo(IReplyContext::TPtr requestContext) const {
    const TCgiParameters& cgi = requestContext->GetCgiParameters();

    if (Config->GetObfuscateKey()) {
        TCgiParameters cgiFixed(cgi);
        TString newKey = GetObfuscatedKey(cgi.Get(Config->GetKeyParameter()));
        cgiFixed.ReplaceUnescaped(Config->GetKeyParameter(), newKey);

        return CheckApikey(cgiFixed);
    }

    return CheckApikey(cgi);
}


IAuthInfo::TPtr TApikeyAuthModule::CheckApikey(const TCgiParameters& cgi) const {
    const TString& apikey = cgi.Get(Config->GetKeyParameter());
    return MakeAtomicShared<TApikeyAuthInfo>(true, apikey, HTTP_OK, Default<TString>());
}


TString TApikeyAuthModule::GetObfuscatedKey(const TString& key) const {
    MD5 md5;

    md5.Update(key.data(), key.size());
    char buf[64];
    TStringBuf stringBuf(md5.End(buf));

    TStringStream ss;
    ss << stringBuf.SubString(0, 8) << '-';
    ss << stringBuf.SubString(8, 4) << '-';
    ss << stringBuf.SubString(12, 4) << '-';
    ss << stringBuf.SubString(16, 4) << '-';
    ss << stringBuf.SubString(20, 12);
    return ss.Str();
}


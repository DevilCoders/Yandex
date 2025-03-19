#include "idna_decode.h"

#include <contrib/libs/libidn/lib/idna.h>
#include <library/cpp/uri/uri.h>

bool IDNAUrlToUtf8(const TString& inUrl, TString& outUrl, bool cutScheme) {
    if (!inUrl.Contains(IDNA_ACE_PREFIX)) {
        return false;
    }
    NUri::TUri url;
    NUri::TState::EParsed st = url.ParseUri(inUrl, NUri::TParseFlags(NUri::TFeature::FeaturesRobot | NUri::TFeature::FeatureNoRelPath));
    if (NUri::TState::ParsedOK != st) {
        return false;
    }
    TStringBuf host = url.GetHost();
    if (!host) {
        // FIXME(mvel): this check seems to be buggy. see r3036586
        return false;
    }
    char* hostUtf8 = nullptr;
    int errcode = idna_to_unicode_8z8z(host.data(), &hostUtf8, 0);

    TMallocHolder<char> holder(hostUtf8);
    if (IDNA_SUCCESS != errcode) {
        return false;
    }

    url.FldMemSet(NUri::TField::FieldHost, hostUtf8);
    if (cutScheme) {
        url.Print(outUrl, NUri::TField::FlagUrlFields & ~NUri::TField::FlagScheme);
    } else {
        url.Print(outUrl);
    }
    return true;
}

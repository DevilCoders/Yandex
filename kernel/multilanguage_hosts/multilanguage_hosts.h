#pragma once

#include "multilang_prefixes.h"

#include <util/generic/yexception.h>
#include <util/generic/strbuf.h>
#include <library/cpp/string_utils/url/url.h>
#include <util/system/maxlen.h>

inline TStringBuf DoPrepareUrlForMultilanguage(TStringBuf url) {
    return CutHttpPrefix(url, true);
}

inline TString& DoEncodeMultilanguageSearchHost(TStringBuf language, TStringBuf host, TString& buf) {
    using namespace NHostMultilangUtils;
    return DoJoinLangPrefixAndHost(DoLang2SearchPrefix(language, buf), host);
}

inline TString& DoEncodeMultilanguageSearchUrl(TStringBuf language, TStringBuf url, TString& buf) {
    return DoEncodeMultilanguageSearchHost(language, DoPrepareUrlForMultilanguage(url), buf);
}

inline bool IsMultilanguageHost(TStringBuf host) {
    if (host.size() < 2 || host[0] != '@') {
        return false;
    }

    return host.find('.', 1) != TStringBuf::npos;
}

inline TString EncodeMultilanguageHost(const ELangRegion langRegion, const TString& host, const ELangRegion defaultLangRegion) {
    if (IsMultilanguageHost(host))
        ythrow yexception() << "Trying to encode multilanguage host";
    if (langRegion != defaultLangRegion) {
        return GetRobotMultilangPrefix(langRegion) + host;
    } else {
        return host;
    }
}

inline TString EncodeMultilanguageHost(TStringBuf language, TStringBuf host) {
    if (IsMultilanguageHost(host))
        ythrow yexception() << "Trying to encode multilanguage host";
    TString res;
    return DoEncodeMultilanguageSearchHost(language, host, res);
}

// we want to have two versions of this function for {char*, const char*}
template <typename T>
inline T* DecodeMultilanguageHost(T* encodedHost) {
    if (!IsMultilanguageHost(encodedHost))
        ythrow yexception() << "Trying to decode not multilanguage host " << encodedHost;
    T* positionOfPoint = strchr(encodedHost + 1, '.');
    if (positionOfPoint == nullptr)
        ythrow yexception() << "Trying to decode not multilanguage host " << encodedHost;
    return positionOfPoint + 1;
}

inline TString GetMultilanguagePrefix(const char* encodedHost, bool withSeparators = true) {
    if (!IsMultilanguageHost(encodedHost)) {
        return "";
    }

    const char* decodedHost = DecodeMultilanguageHost(encodedHost);
    size_t prefixLen = decodedHost - encodedHost;

    char resultBuf[URL_MAX + 1];

    if (withSeparators) {
        strncpy(resultBuf, encodedHost, prefixLen);
        resultBuf[prefixLen] = 0;
    } else {
        strncpy(resultBuf, encodedHost + 1, prefixLen - 2);
        resultBuf[prefixLen - 2] = 0;
    }

    return resultBuf;
}

inline void DecodeMultilanguageHost(const char* encodedHost, char* decodedHost, char* language,
                                    bool languageWithSeparators = false) {
    if (!IsMultilanguageHost(encodedHost))
        ythrow yexception() << "Trying to decode not multilanguage host";

    const char* pointPtr = strchr(encodedHost + 1, '.');
    size_t positionOfPoint = pointPtr - encodedHost;

    if (pointPtr == nullptr)
        ythrow yexception() << "Trying to decode not multilanguage host";

    strcpy(decodedHost, pointPtr + 1);

    if (!languageWithSeparators) {
        strncpy(language, encodedHost + 1, positionOfPoint - 1);
        *(language + positionOfPoint - 1) = '\0';
    } else {
        strncpy(language, encodedHost, positionOfPoint + 1);
        *(language + positionOfPoint + 1) = '\0';
    }
}

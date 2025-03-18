#include <library/cpp/html/entity/htmlentity.h>
#include <util/system/maxlen.h>

#include "url.h"

namespace NHtml {
    ::NUri::TUri::TLinkType NormalizeLink(const ::NUri::TUri& base, TStringBuf link, ::NUri::TUri* result, ECharset idnaEnc, ECharset decodeEnc) {
        // HTML preprocessing
        char buffer[URL_MAX + 10]; // may be use URL_MAXLEN or TTempBuf

        // decode entities and national alphabets (recode to utf-8 if decodeEnc was supplied)
        size_t buflen;
        if (!HtLinkDecode(link, buffer, sizeof(buffer), buflen, decodeEnc))
            return ::NUri::TUri::LinkIsBad;

        // select idna charset and rewrite national host
        if (decodeEnc != CODES_UNKNOWN)
            idnaEnc = CODES_UTF8;
        else if (idnaEnc == CODES_UNKNOWN)
            idnaEnc = CODES_ASCII;

        const long urlParseFlags = ::NUri::TUri::FeaturesRobot | ::NUri::TUri::FeatureConvertHostIDN | ::NUri::TUri::FeatureHashBangToEscapedFragment;
        return result->Normalize(base, TStringBuf(buffer, buflen), nullptr, urlParseFlags, idnaEnc);
    }

}

#include "host.h"

#include <library/cpp/uri/uri.h>

#include <util/generic/string.h>
#include <util/string/cast.h>

TString ExtractHost(const TStringBuf& url) {
    if (!url)
        return "";

    NUri::TUri parser;
    if (parser.Parse(url) != NUri::TState::ParsedOK) {
        return ToString(url);
    }

    TString host = ToString(parser.GetField(NUri::TField::FieldHost));
    return host;
}

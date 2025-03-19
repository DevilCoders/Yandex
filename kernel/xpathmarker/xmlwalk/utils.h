#pragma once

#include "xmlwalk.h"

#include <libxml/tree.h>

#include <library/cpp/json/json_value.h>
#include <library/cpp/regex/libregex/regexstr.h>
#include <library/cpp/xml/doc/xmltypes.h>

#include <util/generic/string.h>

namespace NHtmlXPath {

class TCapture {
public:
    TCapture(const TString& regexp);

    bool Capture(const TStringBuf& str, TString& result);
    void operator () (unsigned /* matchNo */, const char* start, size_t length);
private:
    TAutoPtr<TRegexStringParser> Parser;
    const char* MatchStart;
    size_t MatchLength;
};

void ApplyRegexpIfAny(TString& data, const TString& regexp);

TString SerializeToStroka(const NXml::TxmlXPathObjectPtr attributeData, const TString& separator = " ", const TString& regexp = TString());

} // NHtmlXPath


#pragma once

#include <library/cpp/html/pdoc/pds.h>

namespace NMango {
    TString FromTExtent(const char *text, const NHtml::TExtent &e);
    TString GetAttributeValue(const TString &attrName, const THtmlChunk &chunk);
    TString GetAttributeValueFromStyle(const TString &attrName, const TString &style);
    bool HasAttribute(const TString &attrName, const THtmlChunk &chunk);
}

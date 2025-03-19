#include "html.h"

#include <library/cpp/charset/codepage.h>

namespace NMango {
    TString FromTExtent(const char *text, const NHtml::TExtent &e) {
        return TString(text + e.Start, e.Leng);
    }

    TString GetAttributeValue(const TString &attrName, const THtmlChunk &chunk) {
        for (size_t i = 0; i < chunk.AttrCount; i++) {
            const NHtml::TAttribute &attr = chunk.Attrs[i];
            if (FromTExtent(chunk.text, attr.Name) == attrName) {
                return FromTExtent(chunk.text, attr.Value);
            }
        }
        return TString();
    }

    TString GetAttributeValueFromStyle(const TString &attrName, const TString &style) {
        size_t start = style.find(attrName + ":");
        while (start != TString::npos && start > 0 && (IsAlnum(style[start - 1]) || style[start - 1] == '-')) {
            start = style.find(attrName + ":", start + attrName.length());
        }
        if (start == TString::npos)
            return "";
        start += attrName.length() + 1;
        size_t finish = start;
        while (finish < style.length() && style[finish] != ';') {
            ++finish;
        }
        return style.substr(start, finish - start);
    }

    bool HasAttribute(const TString &attrName, const THtmlChunk &chunk) {
        for (size_t i = 0; i < chunk.AttrCount; i++) {
            const NHtml::TAttribute &attr = chunk.Attrs[i];
            if (FromTExtent(chunk.text, attr.Name) == attrName) {
                return true;
            }
        }
        return false;
    }
}

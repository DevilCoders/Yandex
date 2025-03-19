#pragma once

#include <util/generic/string.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>

namespace NHtmlXPath {

enum EAttributeType {
    AT_TEXT,
    AT_INTEGER,
    AT_BOOLEAN,
    AT_DATE
};

inline EAttributeType GetType(const TString& type) {
    if (type == "text" || type == "null" || type.empty()) {
        return AT_TEXT;
    }
    if (type == "int" || type == "integer") {
        return AT_INTEGER;
    }
    if (type == "date") {
        return AT_DATE;
    }
    if (type == "bool" || type == "boolean") {
        return AT_BOOLEAN;
    }
    ythrow yexception() << "Unknown attribute type (" << type << "), can't convert to EAttributeType";
}

} //namespace NHtmlXPath

template<>
inline void Out<NHtmlXPath::EAttributeType>(IOutputStream& o, NHtmlXPath::EAttributeType attributeType) {
    switch (attributeType) {
        case NHtmlXPath::AT_TEXT:
            o << "Text attribute";
            break;
        case NHtmlXPath::AT_INTEGER:
            o << "Integer attribute";
            break;
        case NHtmlXPath::AT_DATE:
            o << "Date attribute";
            break;
        case NHtmlXPath::AT_BOOLEAN:
            o << "Boolean attribute";
            break;
        default:
            o << "Unknown-type attribute";
    }
}


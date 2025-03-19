#include "attribute.h"

#include <util/generic/map.h>
#include <util/generic/string.h>

namespace {
    const TMap<char, char> Brackets = {
        { '\"', '\"' },
        { '\'', '\'' },
        { '(', ')' }
    };
}

TStringBuf NUtil::GetAttributeValue(const TStringBuf& name, TStringBuf query) {
    const TString attribute = TString(name) + ":";
    auto attrPosition = query.find(attribute);
    if (attrPosition == TStringBuf::npos) {
        return {};
    }

    query.Skip(attrPosition + attribute.size());

    char limiting = 0;
    auto b = Brackets.find(query[0]);
    if (b != Brackets.end()) {
        limiting = b->second;
        query.Skip(1);
    } else {
        limiting = ' ';
    }

    auto limitingPosition = query.find(limiting);
    if (limitingPosition != TStringBuf::npos) {
        query.Trunc(limitingPosition);
    }

    return query;
}

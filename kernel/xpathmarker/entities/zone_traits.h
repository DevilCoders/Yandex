#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <util/generic/yexception.h>

namespace NHtmlXPath {

enum EExportType {
    ET_NONE          = 0,
    ET_ARCHIVE_ZONES = 1,
    ET_JSON          = 2,
};

inline EExportType ParseExportTypes(const TStringBuf& exportTypes) {
    TStringBuf leftToParse(exportTypes);

    int result = ET_NONE;
    while (! leftToParse.empty()) {
        TStringBuf thisExportType = leftToParse.NextTok(',');
        if (thisExportType == "json") {
            result |= ET_JSON;
        } else if (thisExportType == "archive") {
            result |= ET_ARCHIVE_ZONES;
        } else {
            ythrow yexception() << "unknown export type " << thisExportType;
        }
    }

    if (result == ET_NONE) {
        // compatibility fallback
        result = ET_ARCHIVE_ZONES;
    }

    return EExportType(result);
}

} // namespace NHtmlXPath


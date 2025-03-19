#pragma once

#include "attribute.h"
#include "zone_traits.h"

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NHtmlXPath {

struct TZone {
    TString Name;
    EExportType ExportType;
    TPosting StartPosition;
    TPosting EndPosition;
    TAttributes Attributes;

    TZone(const TString& name, EExportType exportType, TPosting startPosition = 0, TPosting endPosition = 0, const TAttributes& attributes = TAttributes())
        : Name(name)
        , ExportType(exportType)
        , StartPosition(startPosition)
        , EndPosition(endPosition)
        , Attributes(attributes)
    {
        if (Name.empty()) {
            ythrow yexception() << "Zone must have non-empty name";
        }
    }
};

} // namespace NHtmlXPath


#include "util.h"

#include "countries.h"
#include "geodb.h"

#include <util/generic/algorithm.h>
#include <util/generic/string.h>
#include <util/string/builder.h>

TString NGeoDB::SpanAsString(const TGeoPtr& geo, const bool widthFirst) {
    if (widthFirst) {
        return TStringBuilder() << geo->GetSpan().GetWidth() << ',' << geo->GetSpan().GetHeight();
    }

    return TStringBuilder() << geo->GetSpan().GetHeight() << ',' << geo->GetSpan().GetWidth();
}

TString NGeoDB::SpanAsString(const TGeoPtr& geo) {
    return SpanAsString(geo, false);
}

TString NGeoDB::LocationAsString(const TGeoPtr& geo, bool latFirst) {
    if (latFirst) {
        return TStringBuilder() << geo->GetLocation().GetLat() << ',' << geo->GetLocation().GetLon();
    }

    return TStringBuilder() << geo->GetLocation().GetLon() << ',' << geo->GetLocation().GetLat();
}

TString NGeoDB::LocationAsString(const TGeoPtr& geo) {
    return LocationAsString(geo, false);
}

bool NGeoDB::IsKUBR(const TGeoPtr& geo) {
    const TCateg country = geo->GetCountryId();
    return EqualToOneOf(country, RUSSIA_ID, UKRAINE_ID, BELARUS_ID, KAZAKHSTAN_ID);
}

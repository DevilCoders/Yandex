#include "area.h"

#include <kernel/common_server/library/scheme/fields.h>

NCS::NScheme::TScheme TGeoAreaId::GetScheme() {
    NCS::NScheme::TScheme scheme;
    scheme.Add<NCS::NScheme::TFSString>("type", "Geoarea type").SetRequired(true);
    scheme.Add<NCS::NScheme::TFSString>("name", "Geoarea name").SetRequired(true);
    return scheme;
}

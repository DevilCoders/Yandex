#include "info.h"

#include <util/string/cast.h>

namespace NReMorph {

const TRemorphInfo REMORPH_INFO = {
        {REMORPH_VERSION_MAJOR, REMORPH_VERSION_MINOR, REMORPH_VERSION_PATCH}
    };

} // NReMorph

TString ToString(const NReMorph::TRemorphVersion& version) {
    return ::ToString(version.Major) + '.' + ::ToString(version.Minor) + '.' + ::ToString(version.Patch);
}

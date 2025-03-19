#include "fingerprint.h"

#include <util/digest/city.h>
#include <util/digest/numeric.h>

#include <util/stream/output.h>

namespace NTurboLogin {
    ui64 GetFingerPrint(const TString& userAgent, const TString& ip) {
        return CombineHashes(CityHash64(userAgent), CityHash64(ip));
    }
}

#include <library/cpp/digest/crc32c/crc32c.h>
#include <util/string/cast.h>

#include "binary_dict_header.h"

TString NNewLemmer::TData::Fingerprint() const {
    if (Has(F_FINGERPRINT)) {
        return TString((*this)[F_FINGERPRINT], Len(F_FINGERPRINT));
    } else {
        return ToString(Crc32c(Data, Size));
    }
}

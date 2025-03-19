#include "constants.h"

#include <util/string/join.h>

namespace NIznanka {
    namespace NConstants {
        TString DotSearchProps(const TVector<TString>& searchProps) {
            return JoinRange(".", searchProps.begin(), searchProps.end());
        }

        TString UnderscoreSearchProps(const TVector<TString>& searchProps) {
            return JoinRange("_", searchProps.begin(), searchProps.end());
        }
    }
}

#pragma once
#include <util/generic/strbuf.h>

namespace NBlender::NSaasKnn {

    struct TKnnBaseFieldNames {
        static constexpr TStringBuf IntSrpFieldName = "int_srp";
        static constexpr TStringBuf FactorsGroupName = "factors";
    };

}

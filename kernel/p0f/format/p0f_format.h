#pragma once

#include <kernel/p0f/bpf/p0f_bpf_types.h>

#ifdef __cplusplus
    #include <util/generic/maybe.h>
    #include <util/generic/fwd.h>

namespace NP0f {
    struct TP0fOrError {
        TMaybe<TString> Value;
        TMaybe<TString> Error;
    };

    TP0fOrError CreateP0fError(TString error);
    TP0fOrError FormatP0f(const p0f_value_t& p);
}
#endif

#pragma once

#include "qd_rawkeys.h"
#include <kernel/querydata/cgi/qd_request.h>

namespace NQueryData {

    bool FillSubkeysCache(TSubkeysCache&, TKeyTypes&, const TRequestRec&, TStringBuf nspace);
    bool NormalizeQuery(TRawKeys& k, const TRequestRec& r, TStringBuf nspace = TStringBuf());
    bool NormalizeQuery(TRawKeys&, const TRequestRec&, const TStringBufs&, TStringBuf nspace = TStringBuf());

}

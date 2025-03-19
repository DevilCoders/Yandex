#pragma once

#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NQueryData {

    bool QueryIsCommon(TStringBuf q);

    TStringBuf SkipN(TStringBuf& data, ui32 n, char sep = '\t');

    TStringBuf UnescapeKey(TStringBuf key, int keytype, const TKeyTypes& subkeytypes, TString& keybuf);

}

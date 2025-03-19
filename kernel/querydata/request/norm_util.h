#pragma once

#include <kernel/querydata/common/querydata_traits.h>

#include <util/generic/deque.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NQueryData {

    bool PushBackPooled(TStringBufs& subkeys, TDeque<TString>& pool, const TString& str);

    void GetTokens(TStringBufs& normquery, TStringBuf ss, char delim);

    void GetPairs(TStringBufs& normquery, TDeque<TString>& pool, TStringBuf s0, char delim);

}

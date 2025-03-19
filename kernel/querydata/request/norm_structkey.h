#pragma once

#include <kernel/querydata/common/querydata_traits.h>

#include <library/cpp/scheme/scheme.h>

#include <util/string/vector.h>

namespace NQueryData {

    int GetStructKeys(TStringBufs& subkeys, TDeque<TString>& pool, const NSc::TValue& skeys, TStringBuf nspace);

}

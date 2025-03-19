#pragma once

#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace NQueryData {

    class TQueryData;

    void DumpQDRaw(IOutputStream& out, TStringBuf req, const TQueryData& qd, bool values = false);

    void DumpQD(IOutputStream& out, TStringBuf req, const TQueryData& qd, bool values = false);
    void DumpSC(IOutputStream& out, TStringBuf req, const NSc::TValue& sc, bool values = false);

}

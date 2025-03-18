#pragma once

#include <tools/printwzrd/lib/options.h>

#include <util/charset/utf8.h>
#include <util/generic/string.h>
#include <util/generic/strbuf.h>
#include <library/cpp/cgiparam/cgiparam.h>

namespace NPrintWzrd {
    void PrintFailInfo(IOutputStream& out, const TString& request, bool StripLineNumbers);
    void ParseRequest(TStringBuf request, const TPrintwzrdOptions& options, TCgiParameters& cgiParams, TStringBuf& rulesToPrint);
    TCgiParameters MakeCgiParams(TStringBuf request, const TPrintwzrdOptions& options);
    bool PrepareRequest(TString& req, bool tabbedInput, IOutputStream& message);
}

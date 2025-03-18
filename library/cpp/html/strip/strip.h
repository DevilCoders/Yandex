#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

namespace NHtml {
    //does not decode &nbsp;, cause its handling depends on input encoding
    TString StripHtml(const TStringBuf& input);
}

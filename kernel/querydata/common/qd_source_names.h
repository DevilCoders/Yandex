#pragma once

#include <util/generic/strbuf.h>

namespace NQueryData {

    TStringBuf /*mainSrc*/ GetMainSourceName(const TStringBuf& src, TStringBuf& subSrc);
    TStringBuf /*mainSrc*/ GetMainSourceName(const TStringBuf& src);

    TString ComposeSourceName(TStringBuf mainSrc, TStringBuf subSrc);

}

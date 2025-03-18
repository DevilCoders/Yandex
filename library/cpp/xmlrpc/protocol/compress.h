#pragma once

#include <util/generic/string.h>
#include <util/generic/strbuf.h>

namespace NXmlRPC {
    TString CompressXml(const TStringBuf& s);
    TString DecompressXml(const TStringBuf& s);
}

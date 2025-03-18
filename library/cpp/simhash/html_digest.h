#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>

struct THtmlDigest {
    TString StrongDigest;
    TString WeakDigest;
};

THtmlDigest GetHtmlDigest(TStringBuf html);

#pragma once

#include <dict/light_syntax/simple/simple.h>

#include <util/generic/strbuf.h>
#include <util/generic/vector.h>
#include <util/stream/output.h>

namespace NSyntaxer {

class TPrinter {
    IOutputStream& Output;

public:
    TPrinter(IOutputStream& output);

    void Print(const TVector<NLightSyntax::TSimplePhrase>& phrases, const TWtringBuf& text) const;
};

} // NSyntaxer

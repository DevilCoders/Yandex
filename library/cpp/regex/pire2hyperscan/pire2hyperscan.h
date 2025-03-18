#pragma once

#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/yexception.h>

namespace Pire {
    class Lexer;
}

namespace NPire {
    typedef Pire::Lexer TLexer;
}

TString PireLexer2Hyperscan(NPire::TLexer& lexer);
TString PireRegex2Hyperscan(const TStringBuf& regex);

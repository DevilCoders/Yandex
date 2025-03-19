#pragma once

#include "ast.h"
#include "tokens.h"
#include "parser_impl.h"

namespace NRemorph {

template <class TLiteralTable>
TAstNodePtr Parse(const TLiteralTable& lt, const TVectorTokens& tokens) {
    try {
        return NPrivate::ParseImpl(lt, tokens);
    } catch (TUnexpectedToken& e) {
        e.ExpandToken(lt);
        throw;
    }
}

} // NRemorph

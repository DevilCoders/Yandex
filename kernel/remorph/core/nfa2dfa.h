#pragma once

#include "nfa2dfa_impl.h"
#include "dfa.h"
#include "nfa.h"

namespace NRemorph {

template <class TLiteralTable>
inline TDFAPtr Convert(const TLiteralTable& lt, const TNFA& nfa) {
    NPrivate::Converter c(lt, nfa);
    return c.Result;
}

template <class TLiteralTable>
TDFAPtr Convert(const TLiteralTable& lt, const TVectorNFAs& nfas);

} // NRemorph

#pragma once

#include "compiler_impl.h"
#include "ast.h"
#include "nfa.h"
#include "dfa.h"
#include "tokens.h"
#include "parser.h"
#include "nfa2dfa.h"

namespace NRemorph {

TNFAPtr Combine(const TVectorNFAs& nfas);

template <class TLiteralTable>
inline TNFAPtr CompileNFA(const TLiteralTable& lt, TAstNode* node) {
    TNFAPtr nfa = new TNFA();
    nfa->NumSubmatches = NPrivate::CalcSubmatchIds(lt, node, nfa->SubmatchIdToName, nfa->SubmatchIdToSourcePos);
    NPrivate::Compile(nfa, node);
    return nfa;
}

template <class TLiteralTable>
inline TNFAPtr CompileNFA(const TLiteralTable& lt, const TVectorTokens& tokens) {
    return CompileNFA(lt, Parse(lt, tokens).Get());
}

template <class TLiteralTable>
inline TDFAPtr CompileDFA(const TLiteralTable& lt, const TVectorTokens& tokens) {
    return Convert(lt, *CompileNFA(lt, Parse(lt, tokens).Get()));
}

template <class TLiteralTable>
inline TDFAPtr Convert(const TLiteralTable& lt, const TVectorNFAs& nfas) {
    TNFAPtr nfa = Combine(nfas);
    return Convert(lt, *nfa);
}

} // NRemorph

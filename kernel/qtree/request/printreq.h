#pragma once

#include <stdio.h>

#include <util/generic/string.h>
#include <util/string/printf.h>
#include <util/charset/wide.h>

#include "req_node.h"

//-------------------------- LEX constants: ----------------------------
extern const char * LEFT_PAREN_STR;
extern const char * RIGHT_PAREN_STR;

extern const char * AND_STR;
extern const char * AND_NOT_STR;
extern const char * OR_STR;
extern const char * WEAK_OR_STR;
extern const char * REFINE_STR;
extern const char * RESTR_DOC_STR;

extern const char * NEAR_PREFFIX_STR;
extern const char * REVERSE_FREQ_PREFFIX_STR;

extern const char * CMP_LE_STR;
extern const char * CMP_GE_STR;

extern const char * NEW_CMP_LT_STR;
extern const char * NEW_CMP_LE_STR;
extern const char * NEW_CMP_EQ_STR;
extern const char * NEW_CMP_GE_STR;
extern const char * NEW_CMP_GT_STR;

extern const char * EXACT_WORD_PREFFIX_STR;
extern const char * EXACT_LEMMA_PREFFIX_STR;
extern const char * NEAR_DELIM_STR;

extern const char * BEG_ATTRVALUE_STR;

extern const char * BEG_RESTRBYPOS_STR;
extern const char * END_RESTRBYPOS_STR;
extern const char * COMMA_STR;


//----------------------------------------------------------------------
inline TString& printNearSpec(const TOpInfo& op, TString& res) {
   if (op.Lo || op.Hi || !op.Level) {
      res += Sprintf("%s%s%ld %ld%s", NEAR_PREFFIX_STR, LEFT_PAREN_STR, (long)op.Lo, (long)op.Hi, RIGHT_PAREN_STR);
   }
   return res;
}

inline TString& printRestrictBounds(const TOpInfo& op, TString& res) {
   res += Sprintf("%s %ld%s %ld", COMMA_STR, (long)op.Lo, COMMA_STR, (long)op.Hi);
   return res;
}

inline TString& printReverseFreq(const TRequestNode* n, TString& res) {
   if (n && n->ReverseFreq >= 0) {
      res += Sprintf("%s%ld", REVERSE_FREQ_PREFFIX_STR, (long)n->ReverseFreq);
   }
   return res;
}

//! @param newAttrStyle       @c true if name:value representation is used
inline TString& printCmpOp(const TRequestNode* n, TString& res) {
    if (!n)
        return res;
    switch (n->OpInfo.CmpOper) {
    case cmpLT: res += NEW_CMP_LT_STR; break;
    case cmpLE: res += NEW_CMP_LE_STR; break;
    case cmpEQ: res += NEW_CMP_EQ_STR; break;
    case cmpGE: res += NEW_CMP_GE_STR; break;
    case cmpGT: res += NEW_CMP_GT_STR; break;
    }
    return res;
}

inline bool isBinOp(const TRequestNode* n) {
   return n && (
          n->Op() == oAnd || n->Op() == oAndNot || n->Op() == oOr ||
          n->Op() == oRefine || n->Op() == oWeakOr ||
          n->Op() == oRestrDoc);
}

inline bool needParen(const TRequestNode* n) {
    return n && (n->Parens || n->GetSoftness());
}

inline bool needModifier(const TRequestNode* n) {
    return n && (!isBinOp(n) || needParen(n) || n->IsQuoted());
}

inline void printNecessity(const TRequestNode* n, TString& res, TNodeNecessity parN) {
   if (!needModifier(n))
      return;
   if (n->Necessity != parN) {
      if (n->Necessity == nMUST)
         res += "+";
      else if (n->Necessity == nSHOULD)
         res += "%";
   }
}

inline void printFormType(const TRequestNode* n, TString& res, TFormType parFT) {
   if (!needModifier(n))
      return;
   if (n->FormType != parFT) {
      if (n->FormType == fExactLemma)
         res += EXACT_LEMMA_PREFFIX_STR;
      else if (n->FormType == fExactWord)
         res += EXACT_WORD_PREFFIX_STR;
   }
}

//! prints request using new syntax for zones and attributes
//! @return UTF8 encoded text
TString& PrintRequest(const TRequestNode& node, TString& res, bool parentQuoted = false,
                     TFormType parFT = fGeneral, TNodeNecessity parN = nDEFAULT);

//! @return UTF8 encoded word
inline TString& PrintWord(const TRequestNode& node, TString &res, TFormType parFT, TNodeNecessity parN) {
   printNecessity(&node, res, parN);
   printFormType(&node, res, parFT);

   if (node.IsQuoted())
       res += NEAR_DELIM_STR;
   res += WideToUTF8(node.GetText());
   printReverseFreq(&node, res);
   if (node.IsQuoted())
       res += NEAR_DELIM_STR;

   return res;
}

inline TString& PrintSoftness(const TRequestNode& node, TString& res) {
   if (node.GetSoftness()) {
      // it always has the leading space because it never can be at the beginning of the request
      res += Sprintf(" softness:%ld", static_cast<unsigned long>(node.GetSoftness()));
   }
   return res;
}

inline bool NeedsParentheses(const TRequestNode& node) {
    return node.Parens;
}

inline TString& PrintRefineFactor(const TRequestNode& node, TString& res) {
    Y_ASSERT(node.Op() == oRefine);
    if (!node.GetFactorName().empty()) {
        // it always has the leading space because it appears only after refine operator
        res += Sprintf(" %s:%s=%g", ATTR_REFINE_FACTOR_STR, WideToUTF8(node.GetFactorName()).data(), node.GetFactorValue());
    }
    return res;
}

#include "printreq.h"

extern const char * CMP_RIGHT_ARROW; //"->"

const char ATTRVALUE_QUOTES[] = "\"\'`";

static inline bool qHasAttr(const TRequestNode* n) {
    return n && (!!n->GetText() && !!n->GetTextName() || qHasAttr(n->Left) || qHasAttr(n->Right));
}

static inline TString& PrintBinOp(const TOpInfo& op, TString& res) {
   unsigned i, level;

   switch (op.Op) {
    case oAnd:
    case oAndNot:
      level = op.Level;
      if ((op.Lo || op.Hi || !op.Level))
         ++level;
      for (i=0; i<level; ++i) {
         if (op.Op==oAnd)
            res+= AND_STR;
         else
            res+= AND_NOT_STR;
      }
      printNearSpec(op, res);
      break;
    case oOr:
      res += OR_STR;
      break;
    case oWeakOr:
      res += WEAK_OR_STR;
      break;
    case oRefine:
      res += REFINE_STR;
      break;
    case oRestrDoc:
      res += RESTR_DOC_STR;
      break;
    default:
      break;
   }
   return res;
}

TString& PrintRequest(const TRequestNode& node, TString& res, bool parentQuoted, TFormType parFT, TNodeNecessity parN)
{
    if (isBinOp(&node)) {
        // if a node has prefixes, parentheses and quotes - they are written in the follow sequence:
        // [ + - % ] -> [ ! !! ] -> [ ( ] -> [ " ]
        // even if prefix was between ( and " in the request, for example: (!"one two") then it will be changed to !("one two")
        printNecessity(&node, res, parN);
        printFormType(&node, res, parFT);

        if (NeedsParentheses(node))
            res += LEFT_PAREN_STR;

        // use new attribute style if parent attribute style is new and name is known

        const bool hasAttr = qHasAttr(&node);
        if (node.IsQuoted() && !hasAttr)
            res += NEAR_DELIM_STR;

        const bool quoted = node.IsQuoted() ? true : parentQuoted;

        if (node.Left)
            PrintRequest(*node.Left, res, quoted, node.FormType, node.Necessity);

        res += " ";
        if (quoted) {
            Y_ASSERT(node.OpInfo.Lo == node.OpInfo.Hi);
            for (int i = 1; i < node.OpInfo.Lo; ++i) {
                res.append("* ");
            }
        } else {
            if (node.GetPhraseType() != PHRASE_PHRASE) {
                PrintBinOp(node.OpInfo, res);
                if (node.Op() == oRefine)
                    PrintRefineFactor(node, res);
                res += " ";
            }
        }

        if (node.Right)
            PrintRequest(*node.Right, res, quoted, node.FormType, node.Necessity);

        if (node.IsQuoted() && !hasAttr)
            res += NEAR_DELIM_STR;

        if (NeedsParentheses(node))
            res += RIGHT_PAREN_STR;

        printReverseFreq(&node, res);
        PrintSoftness(node, res);
    } else {
        if (IsAttribute(node)) {
            printNecessity(&node, res, parN);
            if (!node.GetAttrValueHi()) {
                // regular attribute has no form type (! or !!)

                res += WideToUTF8(node.GetTextName());
                printCmpOp(&node, res);

                if (!!node.GetText()) {
                    if (strchr(ATTRVALUE_QUOTES, WideToUTF8(node.GetText()).at(0))) {
                        res += WideToUTF8(node.GetText());
                        res += WideToUTF8(node.GetText()).at(0);
                        printReverseFreq(&node, res);
                    } else {
                        res += LEFT_PAREN_STR;
                        printFormType(&node, res, parFT);
                        res += WideToUTF8(node.GetText());
                        printReverseFreq(&node, res);
                        res += RIGHT_PAREN_STR;
                    }
                }
            } else {
                res += LEFT_PAREN_STR;
                bool hasQuote = (!!node.GetText() && node.GetText()[0] == '"');
                res += WideToUTF8(node.GetTextName()) + CMP_GE_STR + WideToUTF8(node.GetText()) + (hasQuote ? BEG_ATTRVALUE_STR : "") + " " + RESTR_DOC_STR + " ";
                res += WideToUTF8(node.GetTextName()) + CMP_LE_STR + WideToUTF8(node.GetAttrValueHi()) + (hasQuote ? BEG_ATTRVALUE_STR : "");
                res += RIGHT_PAREN_STR;
            }
            return res;
        } else if (IsWordOrMultitoken(node)) {
            PrintWord(node, res, parFT, parN);
            res += WideToUTF8(node.GetPunctAfter());
            return res;
        }

        switch (node.Op()) {
        case oZone:
            {
                printNecessity(&node, res, parN);
                printFormType(&node, res, parFT);

                res += WideToUTF8(node.GetTextName());

                if (node.OpInfo.Arrange)
                    res += CMP_RIGHT_ARROW;
                else
                    res += NEW_CMP_EQ_STR;
                res += LEFT_PAREN_STR;
                if (node.Left)
                    PrintRequest(*node.Left, res, 0, node.FormType, node.Necessity);
                if (node.Right) {
                    if (node.Left)
                        res += ' ';
                    PrintRequest(*node.Right, res, 0, node.FormType, node.Necessity);
                }
                res += RIGHT_PAREN_STR;
            }
            break;
        case oRestrictByPos:
            if (node.Left) {
                res += " ";
                res += BEG_RESTRBYPOS_STR;
                PrintRequest(*node.Left, res, 0, node.FormType, node.Necessity);
                printRestrictBounds(node.OpInfo, res);
                res += END_RESTRBYPOS_STR;
            }
            break;
        default:
            break;
        }
    }
    return res;
}

#include "sanit_config.h"

namespace NCssConfig {
    TEssence& TEssence::operator=(const TEssence& copy) {
        if (Expr) {
            delete Expr;
            Expr = nullptr;
        }

        if (copy.Expr)
            Expr = copy.Expr->Clone();

        Regexp = copy.Regexp;
        Text = copy.Text;

        return *this;
    }

    void TEssenceSet::AddEssenceSet(const TEssenceSet& set) {
        Set.insert(set.Set.begin(), set.Set.end());
    }

    void TConfig::InitDefaults() {
        DefaultPass = false;
        ExpressionPass.SetEmpty();
        PropSetPass.Clear();
        PropSetDeny.Clear();
    }

    void TConfig::AddSelectorAppend(const TStrokaList& list) {
        SelectorAppendList.insert(SelectorAppendList.begin() + SelectorAppendList.size(), list.begin(), list.end());
    }

}

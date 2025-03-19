#include "literal_table.h"
#include "logic_expr.h"

#include <util/generic/yexception.h>

namespace NLiteral {

NRemorph::TLiteral TLiteralTableImpl::Add(const TLTElementPtr& e) {
    const TString& name = e->ToString();
    TTable::iterator i = Table.find(name);
    if (i == Table.end()) {
        if (Literals.size() > NRemorph::TLiteral::MAX_ID)
            ythrow yexception() << "Too many literals";
        NRemorph::TLiteral l(Literals.size(), NRemorph::TLiteral::Ordinal);
        Literals.push_back(e);
        Table.insert(std::make_pair(name, l));
        return l;
    }
    return i->second;
}

void TLiteralTableImpl::CollectUsedGztItems(THashSet<TUtf16String>& result) const {
    for (size_t l = 0; l < Literals.size(); ++l) {
        if (Literals[l]->Type == TLTElementBase::Logic) {
            const TLTElementLogic& logic = static_cast<const TLTElementLogic&>(*Literals[l]);
            NLiteral::NPrivate::CollectUsedGztItems(logic.Instr, result);
        }
    }
}

} // NLiteral

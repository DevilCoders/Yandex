#include "matcher_base.h"

#include <kernel/remorph/literal/parser.h>
#include <kernel/remorph/input/input_symbol.h>

#include <util/generic/hash.h>

namespace NMatcher {

using namespace NSymbol;
using namespace NLiteral;

TMatcherBase::~TMatcherBase() {
}

void TMatcherBase::Init(const NGzt::TGazetteer* gzt) {
    THashSet<TUtf16String> gztItems;
    CollectUsedGztItems(gztItems);
    if (!gztItems.empty()) {
        if (nullptr == gzt) {
            throw yexception() << "Loaded rules require the Gazetteer instance to be specified";
        }
        UsedGztItems.Add(*gzt, gztItems);
    }
}

namespace {
    inline TExpressionId GetNextExpressionId(const TString& exp, THashMap<TString, TExpressionId>& expMap) {
        THashMap<TString, TExpressionId>::iterator i = expMap.find(exp);
        if (i == expMap.end()) {
            return expMap.insert(std::make_pair(exp, static_cast<TExpressionId>(expMap.size()))).first->second;
        }
        return i->second;
    }
}

void TMatcherBase::SetFilter(const TString& filter, const NGzt::TGazetteer* gzt) {
    if (filter.empty()) {
        Filter.clear();
        return;
    }

    try {
        Filter = NLiteral::ParseLogic(filter, THashMap<TString, NLiteral::TLInstrVector>());
    } catch (const NLiteral::TLiteralParseError& e) {
        throw yexception() << "Bad filter value: " << e.what();
    }

    if (Filter.empty())
        return;

    if (1 == Filter.size() && NLiteral::TLInstrBasePtr::Gzt == Filter.front().Type) {
        if (nullptr == gzt)
            throw yexception() << "Specified filter require the Gazetteer instance to be specified";
        FilterArticles.Add(*gzt, Filter.front().GetGzt().GztArts);
        return;
    }

    THashSet<TUtf16String> gztItems;
    NLiteral::NPrivate::CollectUsedGztItems(Filter, gztItems);
    if (!gztItems.empty()) {
        if (nullptr == gzt) {
            throw yexception() << "Specified filter require the Gazetteer instance to be specified";
        }
        UsedGztItems.Add(*gzt, gztItems);
    }
    THashMap<TString, NSymbol::TExpressionId> expMap;
    for (auto& instr : Filter) {
        switch (instr.Type) {
#define X(A) case NLiteral::TLInstrBasePtr::A:
        CACHEABLE_LOGIC_EXPR_INSTR_LIST
#undef X
            instr.ExpId = GetNextExpressionId(ToString(instr), expMap);
            break;
        default:
            break;
        }
    }
}

} // NMatcher

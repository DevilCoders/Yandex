#pragma once

#include "ltelement.h"
#include "agreement.h"

#include <kernel/remorph/core/core.h>
#include <kernel/remorph/input/input_symbol.h>

namespace NLiteral {

using namespace NSymbol;

class TLiteralTableImpl {
protected:
    typedef THashMap<TString, NRemorph::TLiteral> TTable;

protected:
    TVector<TLTElementPtr> Literals;
    TTable Table;
    THashMap<NRemorph::TLiteral, TAgreementGroupPtr> Agreements;

public:
    NRemorph::TLiteral Add(const TLTElementPtr& e);

    inline const TLTElementBase* Get(NRemorph::TLiteral l) const {
        Y_ASSERT(l.IsOrdinal());
        return Literals[l.GetId()].Get();
    }

    template <class TSymbolPtr>
    inline bool IsEqual(NRemorph::TLiteral l, const TSymbolPtr& s, TDynBitMap& ctx) const {
        Y_ASSERT(l.IsOrdinal());
        return ::NLiteral::NPrivate::IsEqual(*Literals[l.GetId()], *s, ctx);
    }

    inline TString ToString(NRemorph::TLiteral l) const {
        Y_ASSERT(l.IsOrdinal());
        return Literals[l.GetId()]->ToString() + "+" + ::ToString(GetPriority(l));
    }
    inline void Save(IOutputStream& out) const {
        ::Save(&out, Literals);
        ::Save(&out, Agreements);
    }
    inline void Load(IInputStream& in) {
        ::Load(&in, Literals);
        ::Load(&in, Agreements);
    }
    inline NRemorph::TLiteralId Size() const {
        return (NRemorph::TLiteralId)Literals.size();
    }
    inline NRemorph::TLiteralId GetPriority(NRemorph::TLiteral l) const {
        if (Literals[l.GetId()]->Type == TLTElementBase::Single) {
            // simple words have highest priority
            return 0;
        }
        // all the others have priority in order of appearance in rules file
        return l.GetId() + 1;
    }

    bool CheckAgreements(NRemorph::TLiteral l, const TInputSymbolPtr& s, TDynBitMap& ctx, TAgreementContext& context) const {
        if (l.IsAny())
            return true;
        THashMap<NRemorph::TLiteral, TAgreementGroupPtr>::const_iterator i = Agreements.find(l);
        return i == Agreements.end() || i->second->Check(s, ctx, context);
    }

    void CopyAgreements(NRemorph::TLiteral oldLit, NRemorph::TLiteral newLit) {
        Y_ASSERT(!oldLit.IsAny() && !newLit.IsAny());
        THashMap<NRemorph::TLiteral, TAgreementGroupPtr>::const_iterator i = Agreements.find(oldLit);
        if (i != Agreements.end()) {
            TAgreementGroupPtr& newAgr = Agreements[newLit];
            if (!newAgr) {
                newAgr = new TAgreementGroup();
            }
            newAgr->AddAgreements(i->second->GetAgreements());
        }
    }

    TAgreementGroup& GetAgreementGroup(NRemorph::TLiteral l) {
        Y_ASSERT(!l.IsAny() && !l.IsAnchor() && !l.IsNone());
        TAgreementGroupPtr& agr = Agreements[l];
        if (!agr) {
            agr = new TAgreementGroup();
        }
        return *agr;
    }

    void CollectUsedGztItems(THashSet<TUtf16String>& result) const;
};

typedef NRemorph::TLiteralTable<TLiteralTableImpl> TLiteralTable;

typedef TSimpleSharedPtr<TLiteralTable> TLiteralTablePtr;

} // NLiteral

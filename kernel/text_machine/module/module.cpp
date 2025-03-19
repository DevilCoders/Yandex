#include "module.h"

namespace NModule {
    TMethodTemplateArgInfo::TMethodTemplateArgInfo(
        TStringBuf name,
        const NLingBoost::IIntegralType* descr)
        : Name(name)
        , Descr(descr)
    {
        Y_VERIFY(descr);
    }

    TStringBuf TMethodTemplateArgInfo::GetName() const {
        return Name;
    }
    const NLingBoost::IIntegralType& TMethodTemplateArgInfo::GetDescr() const {
        Y_ASSERT(Descr);
        return *Descr;
    }

    void TMethodTemplateArgInfo::SetInstantiations(TVector<int>&& values) {
        SortUnique(values);
        InstantiatedValues.swap(values);
    }

    const TVector<int>& TMethodTemplateArgInfo::GetInstantiations() const {
        return InstantiatedValues;
    }

    void TMethodTemplateArgInfo::SetAlwaysForward(bool value) {
        AlwaysForward = value;
    }

    bool TMethodTemplateArgInfo::GetAlwaysForward() const {
        return AlwaysForward;
    }

    TUnitMethodInfo::TUnitMethodInfo(TStringBuf name)
        : Name(name)
    {
    }

    TStringBuf TUnitMethodInfo::GetName() const {
        return Name;
    }

    void TUnitRegistry::RegisterUnit(THolder<IUnitInfo>&& info) {
        Y_VERIFY(info);
        Y_VERIFY(!info->GetName().empty());
        UnitByName[TUnitId{info->GetDomainName(), info->GetName()}] = std::move(info);
    }

    const IUnitInfo* TUnitRegistry::GetUnitInfo(TStringBuf domainName, TStringBuf unitName) const {
        if (auto ptr = UnitByName.FindPtr(TUnitId{domainName, unitName})) {
            return ptr->Get();
        }
        return nullptr;
    }
} // NModule

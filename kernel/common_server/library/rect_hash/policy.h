#pragma once

#include <kernel/common_server/library/geometry/rect.h>

#include <library/cpp/logger/global/global.h>

#include <util/generic/ptr.h>

template <class TCoord>
class TCommonHashBuilderPolicy {
private:
    ui32 CriticalObjectsCount = 100;
    bool GlueNecessary = true;
    typename TCoord::TCoordType CriticalLengthDelta = 100;

public:
    TCommonHashBuilderPolicy(ui32 criticalObjectsCount, typename TCoord::TCoordType criticalLengthDelta, const bool glueNecessary)
        : CriticalObjectsCount(criticalObjectsCount)
        , GlueNecessary(glueNecessary)
        , CriticalLengthDelta(criticalLengthDelta)
    {
    }

    Y_FORCE_INLINE TCommonHashBuilderPolicy& SetCriticalLengthDelta(const typename TCoord::TCoordType newLength) {
        CriticalLengthDelta = newLength;
        return *this;
    }

    Y_FORCE_INLINE typename TCoord::TCoordType GetCriticalLengthDelta() const {
        return CriticalLengthDelta;
    }

    Y_FORCE_INLINE ui32 GetCriticalObjectsCount() const {
        return CriticalObjectsCount;
    }

    Y_FORCE_INLINE TCommonHashBuilderPolicy& SetCriticalObjectsCount(const ui32 newCrit) {
        CHECK_WITH_LOG(newCrit);
        CriticalObjectsCount = newCrit;
        return *this;
    }

    Y_FORCE_INLINE bool GetGlueNecessary() const {
        return GlueNecessary;
    }

    Y_FORCE_INLINE TCommonHashBuilderPolicy& SetGlueNecessary(const bool value) {
        GlueNecessary = value;
        return *this;
    }
};

template <class TCoord, class TObj>
class THashBuilderPolicyDefault: public TCommonHashBuilderPolicy<TCoord> {
private:
    using TBase = TCommonHashBuilderPolicy<TCoord>;

public:
    using TBase::GetCriticalObjectsCount;
    using TBase::GetCriticalLengthDelta;
    using TBase::GetGlueNecessary;

    using TBase::SetCriticalObjectsCount;
    using TBase::SetCriticalLengthDelta;
    using TBase::SetGlueNecessary;

    THashBuilderPolicyDefault(ui32 criticalObjectsCount = 100, typename TCoord::TCoordType criticalLengthDelta = 100, const bool glueNecessary = true)
        : TBase(criticalObjectsCount, criticalLengthDelta, glueNecessary)
    {
    }

    bool CheckSplitCondition(const TRect<TCoord>& rect, const TVector<TObj>& objects) const {
        return (rect.Min.X + rect.Min.MakeDXFromDistance(GetCriticalLengthDelta()) < rect.Max.X
                || rect.Min.Y + rect.Min.MakeDYFromDistance(GetCriticalLengthDelta()) < rect.Max.Y)
            && objects.size() > GetCriticalObjectsCount();
    }

    bool CheckGlueCondition(const TRect<TCoord>&, const TVector<TObj>& objects) const {
        return objects.size() < GetCriticalObjectsCount() * 0.75;
    }
};

template <class TCoord, class TObj>
class TGeoSearchHashPolicy: public TCommonHashBuilderPolicy<TCoord> {
private:
    using TBase = TCommonHashBuilderPolicy<TCoord>;

public:
    using TBase::GetCriticalObjectsCount;
    using TBase::GetCriticalLengthDelta;
    using TBase::GetGlueNecessary;

    using TBase::SetCriticalObjectsCount;
    using TBase::SetCriticalLengthDelta;
    using TBase::SetGlueNecessary;

    TGeoSearchHashPolicy(ui32 criticalObjectsCount = 100, double criticalLengthDelta = 100, const bool glueNecessary = true)
        : TBase(criticalObjectsCount, criticalLengthDelta, glueNecessary)
    {}

    bool CheckSplitCondition(const TRect<TCoord>& rect, const TVector<TObj>& objects) const {
        if (objects.size() == 0) {
            return false;
        }
        double square = TCoord::CalcRectArea(rect.Min, rect.Max);
        DEBUG_LOG << "op=split_check;rect=" << rect.ToString() << ";obj=" << objects.size() << ";square=" << square << Endl;
        return (square / objects.size() > GetCriticalLengthDelta() * GetCriticalLengthDelta());
    }

    bool CheckGlueCondition(const TRect<TCoord>& rect, const TVector<TObj>& objects) const {
        if (objects.size() == 0) {
            return true;
        }
        double square = TCoord::CalcRectArea(rect.Min, rect.Max);
        DEBUG_LOG << "op=glue_check;rect=" << rect.ToString() << ";obj=" << objects.size() << ";square=" << square << Endl;
        return square / objects.size() < GetCriticalLengthDelta() * GetCriticalLengthDelta();
    }
};

namespace NPrivate {
    template <class TContainer, class TPolicy>
    class TContainerNormalizer {
    public:
        ~TContainerNormalizer() {
            Container.Normalization();
        }

        TContainerNormalizer(TContainer& container, TPolicy& policy)
            : Container(container)
            , Policy(policy)
        {}

        TPolicy* GetPolicy() {
            return &Policy;
        }

    private:
        TContainer& Container;
        TPolicy& Policy;
    };
}

template <class TContainer, class TPolicy>
class TPolicyGuard : public TAtomicSharedPtr<NPrivate::TContainerNormalizer<TContainer, TPolicy>> {
    using TBase = TAtomicSharedPtr<NPrivate::TContainerNormalizer<TContainer, TPolicy>>;
public:
    TPolicyGuard(TContainer& container, TPolicy& policy)
        : TBase(MakeAtomicShared<NPrivate::TContainerNormalizer<TContainer, TPolicy>>(container, policy))
    {}

    TPolicy* operator->() {
        return TBase::Get()->GetPolicy();
    }

    const TPolicy* operator->() const {
        return TBase::Get()->GetPolicy();
    }

    bool operator!() const {
        return !TBase::Get()->GetPolicy();
    }
};

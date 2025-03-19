#pragma once
#include <util/datetime/base.h>
#include <util/generic/singleton.h>

class TInstantModel {
private:
    TInstant ExternalInstant = TInstant::Max();

    void SetImpl(const TInstant& external) {
        ExternalInstant = external;
    }

    void ResetImpl() {
        ExternalInstant = TInstant::Max();
    }

    TInstant NowImpl(const TInstant nowDef) const {
        if (ExternalInstant != TInstant::Max())
            return ExternalInstant;
        return nowDef;
    }

    bool InitializedImpl() const {
        return (ExternalInstant != TInstant::Max());
    }

public:

    static bool Initialized() {
        return Singleton<TInstantModel>()->InitializedImpl();
    }

    static void Set(const TInstant external) {
        Singleton<TInstantModel>()->SetImpl(external);
    }

    static void Reset() {
        Singleton<TInstantModel>()->ResetImpl();
    }

    static TInstant Now() {
        return Singleton<TInstantModel>()->NowImpl(TInstant::Now());
    }

    static TInstant NowDef(const TInstant nowDef) {
        return Singleton<TInstantModel>()->NowImpl(nowDef);
    }
};

class TInstantGuard {
public:

    TInstantGuard() {

    }

    TInstantGuard(const TInstant& external) {
        TInstantModel::Set(external);
    }

    void Set(const TInstant& external) {
        TInstantModel::Set(external);
    }

    ~TInstantGuard() {
        TInstantModel::Reset();
    }

};

Y_FORCE_INLINE TInstant ModelingNow() {
    return TInstantModel::Now();
}

Y_FORCE_INLINE TInstant ModelingNow(const TInstant defValue) {
    return TInstantModel::NowDef(defValue);
}

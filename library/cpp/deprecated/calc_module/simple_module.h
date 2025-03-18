#pragma once

#include "calc_module.h"
#include "slave_action_points.h"
#include "slave_misc_points.h"
#include "slave_stream_points.h"

#include <library/cpp/deprecated/atomic/atomic.h>

/*
 * TSimpleModule defines only the way of storing the access points.
 */
class TSimpleModule: public ICalcModule, private TNonCopyable {
private:
    struct TAccessPointData {
        IAccessPoint* Point;
        TAccessPointHolder OwnedPoint;
        TAccessPointInfo Info;

        TAccessPointData(IAccessPoint* point)
            : Point(point)
        {
        }
    };
    typedef TMap<TString, TAccessPointData> TAccessPoints;

    const TString Name;
    TAccessPoints AccessPoints;

    template <class TDerived>
    class TSimpleModuleOwnerBinder {
    private:
        TDerived* Owner;
        TSimpleModule* This;

    public:
        TSimpleModuleOwnerBinder(TDerived* owner, TSimpleModule* otherOwner)
            : Owner(owner)
            , This(otherOwner)
        {
        }

        template <class TArg>
        void To(TArg arg, const TString& names) {
            This->AddOwnedAccessPoints(
                names,
                new TSlaveCopyPoint<TArg>(Owner, arg));
        }

        template <void (TDerived::*f)()>
        void To(const TString& names) {
            This->AddOwnedAccessPoints(
                names,
                new TSlaveActionPoint(
                    Owner,
                    &TSlaveActionPoint::TBinderData<TDerived>::template Call<f>));
        }
        template <class TArg, TArg (TDerived::*f)()>
        void To(const TString& names) {
            This->AddOwnedAccessPoints(
                names,
                new TSlaveOutputPoint<TArg>(
                    Owner,
                    &TSlaveOutputPoint<TArg>::template TBinderData<TDerived>::template Call<f>));
        }
        template <class TArg, void (TDerived::*f)(TArg)>
        void To(const TString& names) {
            This->AddOwnedAccessPoints(
                names,
                new TSlaveInputPoint<TArg>(
                    Owner,
                    &TSlaveInputPoint<TArg>::template TBinderData<TDerived>::template Call<f>));
        }
        template <class TArg1, class TArg2, void (TDerived::*f)(TArg1, TArg2)>
        void To(const TString& names) {
            This->AddOwnedAccessPoints(
                names,
                new TSlaveNArgsPoint<TArg1, TArg2>(
                    Owner,
                    &TSlaveNArgsPoint<TArg1, TArg2>::template TBinderData<TDerived>::template Call<f>));
        }
        template <class TArg1, class TArg2, class TArg3, void (TDerived::*f)(TArg1, TArg2, TArg3)>
        void To(const TString& names) {
            This->AddOwnedAccessPoints(
                names,
                new TSlaveNArgsPoint<TArg1, TArg2, TArg3>(
                    Owner,
                    &TSlaveNArgsPoint<TArg1, TArg2, TArg3>::template TBinderData<TDerived>::template Call<f>));
        }
    };

    void AddAccessPoint(const TString& name, IAccessPoint* accessPoint, TAccessPointHolder accessPointHolder);
    TAccessPointData* MutableAccessPointData(const TString& name);
    const TAccessPointData* GetAccessPointData(const TString& name) const;
    void ComplainWrongPointName(const TString& name) const;

protected:
    friend class IAccessPoint;

    void AddAccessPoint(const TString& name, IAccessPoint* accessPoint);
    void AddAccessPoints(const TString& names, IAccessPoint* accessPoint);
    void AddOwnedAccessPoint(const TString& name, TAccessPointHolder accessPoint);
    void AddOwnedAccessPoints(const TString& names, TAccessPointHolder accessPoint);
    void ReplaceAccessPoint(const TString& name, IAccessPoint* accessPoint);

    void AddPointDependency(const TString& clientPoint, const TString& usedPoint);
    void AddPointDependencies(const TString& clientPoints, const TString& usedPoints);
    void AddInitDependency(const TString& name);

    /*
     * Checks internal readiness for work which can be checked only in derivate classes
     */
    virtual void CheckIsInternalReady() const {
    }

public:
    TSimpleModule(const TString& baseModuleName);
    ~TSimpleModule() override;

    /* ICalcModule interface functions */
    TSet<TString> GetPointNames() const override;
    const TString& GetName() const override;
    IAccessPoint& GetAccessPoint(const TString& name) override;
    const TAccessPointInfo* GetAccessPointInfo(const TString& name) const override;
    void CheckReady() const override;

    template <class TDerived>
    TSimpleModuleOwnerBinder<TDerived> Bind(TDerived* owner) {
        return TSimpleModuleOwnerBinder<TDerived>(owner, this);
    }
};

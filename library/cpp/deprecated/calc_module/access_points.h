#pragma once

#include <library/cpp/deprecated/atomic/atomic.h>

#include <util/generic/string.h>
#include <util/generic/yexception.h>

#include <typeinfo>

class TIncompatibleAccessPoints: public yexception {
};

class TTwoSlavePointsException: public TIncompatibleAccessPoints {
};

class TTwoMasterPointsException: public TIncompatibleAccessPoints {
};

class TSimpleModule;

class IAccessPoint {
private:
    void* Owner;
    TAtomicBase ConnectionId;
    TString Names;

protected:
    void RegisterMe(TSimpleModule* module, const TString& names);

    IAccessPoint(void* owner, TAtomicBase connectionId);
    IAccessPoint(const IAccessPoint& accessPoint);

    void* GetOwner() const noexcept {
        return Owner;
    }
    const TString& GetNames() const noexcept {
        return Names;
    }
    void Reset();

public:
    typedef TAtomicBase TConnectionId;

    IAccessPoint& operator=(const IAccessPoint& accessPoint);
    virtual ~IAccessPoint();

    /*
     * Forget any info got through connection
     */
    virtual void Drop() {
    }

    /*
     * Connect two points. Throws exception if something happens.
     */
    void Connect(IAccessPoint& accessPoint);

    /*
     * Get unique connection id. Zero if no connection.
     */
    TConnectionId GetConnectionId() const noexcept {
        return ConnectionId;
    }
};

using TAccessPointHolder = TAtomicSharedPtr<IAccessPoint>;

class TSlaveAccessPoint: public IAccessPoint {
public:
    TSlaveAccessPoint(void* owner);
    TSlaveAccessPoint(const TSlaveAccessPoint& accessPoint);
    TSlaveAccessPoint& operator=(const TSlaveAccessPoint& accessPoint);
    ~TSlaveAccessPoint() override;
};

class TMasterAccessPoint: public IAccessPoint {
private:
    void Drop() override {
        Reset();
        DoDrop();
    }

protected:
    friend void IAccessPoint::Connect(IAccessPoint&);

    /// @throws TIncompatibleAccessPoints
    virtual void CheckCompatibility(const TSlaveAccessPoint& slaveAccessPoint) const = 0;
    virtual void DoConnect(const TSlaveAccessPoint& /*slaveAccessPoint*/) = 0;
    virtual void DoDrop() = 0;

    void ThrowUninitException(TString pointName) const {
        // It will work only on points which are the members of a simple module and initialized in a new way
        ythrow yexception() << pointName << ": Access point not initialized (" << GetNames() << ")";
    }

public:
    TMasterAccessPoint();
    TMasterAccessPoint(const TMasterAccessPoint& accessPoint);
    ~TMasterAccessPoint() override;
};

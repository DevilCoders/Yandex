#include "access_points.h"
#include "simple_module.h"

#include <util/string/vector.h>

namespace {
    TAtomic connectionCounter = 0;
}

IAccessPoint::IAccessPoint(void* owner, TAtomicBase connectionId)
    : Owner(owner)
    , ConnectionId(connectionId)
{
}
IAccessPoint::IAccessPoint(const IAccessPoint& accessPoint)
    : Owner(accessPoint.Owner)
    , ConnectionId(accessPoint.ConnectionId)
    , Names(accessPoint.Names)
{
}

IAccessPoint::~IAccessPoint() = default;

void IAccessPoint::Reset() {
    Owner = nullptr /*NULL*/;
    ConnectionId = 0;
}

void IAccessPoint::RegisterMe(TSimpleModule* module, const TString& names) {
    if (!module) {
        return;
    }
    if (Names.empty()) {
        Names = names;
    } else {
        Names.push_back(',');
        Names += names;
    }

    TVector<TString> namesList = SplitString(names, ",");
    for (size_t i = 0; i < namesList.size(); ++i) {
        module->AddAccessPoint(namesList[i], this);
    }
}

void IAccessPoint::Connect(IAccessPoint& accessPoint) {
    TMasterAccessPoint* master1 = dynamic_cast<TMasterAccessPoint*>(this);
    TMasterAccessPoint* master2 = dynamic_cast<TMasterAccessPoint*>(&accessPoint);
    TSlaveAccessPoint* slave1 = dynamic_cast<TSlaveAccessPoint*>(this);
    TSlaveAccessPoint* slave2 = dynamic_cast<TSlaveAccessPoint*>(&accessPoint);
    if (slave1 && slave2) {
        ythrow TTwoSlavePointsException() << "Connecting two slave access points";
    }
    if (master1 && master2) {
        ythrow TTwoMasterPointsException() << "Connecting two master access points";
    }
    if (!master1 && !slave1 || !master2 && !slave2) {
        ythrow TIncompatibleAccessPoints() << "Unknown type of points to connect";
    }
    TMasterAccessPoint& master = master1 ? *master1 : *master2;
    TSlaveAccessPoint& slave = slave1 ? *slave1 : *slave2;
    master.CheckCompatibility(slave);
    master.Owner = slave.Owner;
    master.ConnectionId = slave.ConnectionId;
    try {
        master.DoConnect(slave);
    } catch (const yexception&) {
        master.Drop();
        throw;
    }
}

TSlaveAccessPoint::TSlaveAccessPoint(void* owner)
    : IAccessPoint(owner, AtomicIncrement(connectionCounter))
{
}

TSlaveAccessPoint::TSlaveAccessPoint(const TSlaveAccessPoint& accessPoint)
    : IAccessPoint(accessPoint)
{
}

TSlaveAccessPoint::~TSlaveAccessPoint() = default;

TMasterAccessPoint::TMasterAccessPoint()
    : IAccessPoint(/*owner=*/nullptr /*NULL*/, /*connectionId=*/0)
{
}

TMasterAccessPoint::TMasterAccessPoint(const TMasterAccessPoint& accessPoint)
    : IAccessPoint(accessPoint)
{
}

TMasterAccessPoint::~TMasterAccessPoint() = default;

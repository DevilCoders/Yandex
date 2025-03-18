#include "simple_module.h"
#include <util/string/vector.h>

void TSimpleModule::AddAccessPoint(const TString& name, IAccessPoint* accessPoint, TAccessPointHolder accessPointHolder) {
    if (!accessPoint) {
        ythrow TAbsentPointException() << "Trying to add absent access point: \"" << name << "\"\n";
    }
    std::pair<TAccessPoints::iterator, bool> res = AccessPoints.insert(std::make_pair(name, accessPoint));
    if (!res.second) {
        ythrow TDuplicatePointException() << "Duplicating already existing point: \"" << name << "\"\n";
    }
    TAccessPointData& data = res.first->second;
    data.Point = accessPoint;
    data.OwnedPoint = accessPointHolder;
}
const TSimpleModule::TAccessPointData* TSimpleModule::GetAccessPointData(const TString& name) const {
    TAccessPoints::const_iterator it = AccessPoints.find(name);
    return it == AccessPoints.end() ? nullptr : &it->second;
}
TSimpleModule::TAccessPointData* TSimpleModule::MutableAccessPointData(const TString& name) {
    TAccessPoints::iterator it = AccessPoints.find(name);
    return it == AccessPoints.end() ? nullptr : &it->second;
}
void TSimpleModule::ComplainWrongPointName(const TString& name) const {
    TString pointNames;
    for (const auto& accessPoint : AccessPoints) {
        pointNames += " \"" + accessPoint.first + "\"";
    }
    ythrow TWrongPointNameException() << "Wrong access point name \"" << name << "\" "
                                      << "in module \"" << Name << "\". "
                                      << "Available point names are:" << pointNames;
}
void TSimpleModule::AddPointDependency(const TString& clientPoint, const TString& usedPoint) {
    TAccessPointData* data = MutableAccessPointData(clientPoint);
    if (!data) {
        ComplainWrongPointName(clientPoint);
    }
    if (!AccessPoints.contains(usedPoint)) {
        ComplainWrongPointName(usedPoint);
    }
    data->Info.AddUsedPoint(usedPoint);
}
void TSimpleModule::AddPointDependencies(const TString& clientPoints, const TString& usedPoints) {
    TVector<TString> clientPointList = SplitString(clientPoints, ",");
    TVector<TString> usedPointList = SplitString(usedPoints, ",");
    for (size_t i = 0; i < clientPointList.size(); ++i) {
        const TString& clientPoint = clientPointList[i];
        for (size_t j = 0; j < usedPointList.size(); ++j) {
            AddPointDependency(clientPoint, usedPointList[j]);
        }
    }
}

void TSimpleModule::AddAccessPoint(const TString& name, IAccessPoint* accessPoint) {
    AddAccessPoint(name, accessPoint, /*accessPointHolder=*/nullptr);
}
void TSimpleModule::AddAccessPoints(const TString& names, IAccessPoint* accessPoint) {
    TVector<TString> namesList = SplitString(names, ",");
    for (size_t i = 0; i < namesList.size(); ++i) {
        AddAccessPoint(namesList[i], accessPoint);
    }
}

void TSimpleModule::AddOwnedAccessPoint(const TString& name, TAccessPointHolder accessPoint) {
    AddAccessPoint(name, accessPoint.Get(), accessPoint);
}
void TSimpleModule::AddOwnedAccessPoints(const TString& names, TAccessPointHolder accessPoint) {
    TVector<TString> namesList = SplitString(names, ",");
    for (size_t i = 0; i < namesList.size(); ++i) {
        AddOwnedAccessPoint(namesList[i], accessPoint);
    }
}

void TSimpleModule::ReplaceAccessPoint(const TString& name, IAccessPoint* accessPoint) {
    if (!accessPoint) {
        ythrow TAbsentPointException() << "Trying to add absent access point: \"" << name << "\"\n";
    }
    TAccessPointData* data = MutableAccessPointData(name);
    if (data) {
        data->Point = accessPoint;
    } else {
        AddAccessPoint(name, accessPoint);
    }
}

void TSimpleModule::AddInitDependency(const TString& name) {
    AddPointDependency("init", name);
}

TSimpleModule::TSimpleModule(const TString& baseModuleName)
    : ICalcModule(baseModuleName)
    , Name(baseModuleName)
{
}
TSimpleModule::~TSimpleModule() = default;

TSet<TString> TSimpleModule::GetPointNames() const {
    TSet<TString> ret;
    for (const auto& accessPoint : AccessPoints) {
        ret.insert(accessPoint.first);
    }
    return ret;
}
const TString& TSimpleModule::GetName() const {
    return Name;
}
IAccessPoint& TSimpleModule::GetAccessPoint(const TString& name) {
    TAccessPointData* data = MutableAccessPointData(name);
    if (!data) {
        ComplainWrongPointName(name);
    }
    Y_VERIFY(data->Point, "Internal error in TSimpleModule logic!");
    return *data->Point;
}

const TAccessPointInfo* TSimpleModule::GetAccessPointInfo(const TString& name) const {
    const TAccessPointData* data = GetAccessPointData(name);
    return data ? &data->Info : nullptr;
}

void TSimpleModule::CheckReady() const {
    for (const auto& accessPoint : AccessPoints) {
        if (!accessPoint.second.Point->GetConnectionId()) {
            ythrow TNotConnectedPointException() << "Point is not connected: " << accessPoint.first;
        }
    }
    CheckIsInternalReady();
}

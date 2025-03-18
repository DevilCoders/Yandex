#pragma once

#include "ip_interval.h"
#include "ip_list.h"

#include <util/generic/map.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAntiRobot {

class TIpListProjId {
public:
    TIpListProjId() = default;
    explicit TIpListProjId(const TString& path);

    void Append(IInputStream& input, bool check = true);
    void Append(const TString& path, bool check = true);

    void Load(IInputStream& in);
    void Load(const TString& fileName);

    void AddAddresses(const TVector<TAddr>& addresses);

    void EnsureNoIntersections() const;

    bool Contains(const TAddr& addr) const;

private:
    bool CheckProjectId(const TAddr& addr) const;

    TIpList IpList;
    TMap<TIpInterval, TVector<ui32>> ProjectIds;
};

} // namespace NAntiRobot

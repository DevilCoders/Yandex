#include "access_point_info.h"

void TAccessPointInfo::AddUsedPoint(const TString& name) {
    UsedPoints.insert(name);
}

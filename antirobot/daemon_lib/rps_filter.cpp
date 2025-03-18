#include "rps_filter.h"

namespace NAntiRobot {
    TInstant TRpsFilter::LastTime = TInstant::Zero();

    TDuration TRequestRecord::GetTimeAlive() const {
        return TRpsFilter::LastTime - FirstRequestTime;
    }

    bool TRequestRecord::IsOutdatedRecord(TDuration rememberFor) const {
        return GetTimeAlive() > rememberFor;
    }

    void TRequestRecord::DecrementRequestsCount(TDuration rememberFor) {
        if (IsOutdatedRecord(rememberFor)) {
            RequestCount = 0;
        } else {
            RequestCount--;
        }
    }

    void TRequestRecord::IncrementRequestsCount(TDuration rememberFor) {
        if (IsOutdatedRecord(rememberFor)) {
            FirstRequestTime = TRpsFilter::LastTime;
            RequestCount = 1;
        } else {
            RequestCount++;
        }
    }

    size_t TRequestRecord::GetRequests() const {
        return RequestCount;
    }

    size_t TRpsFilter::GetActualSize() const {
        TReadGuard guard(Mutex);
        return Map.size();
    }

    float TRpsFilter::CalcRps(const TRequestRecord& rec) const {
        return static_cast<float>(rec.GetRequests()) / Max(rec.GetTimeAlive().Seconds(), MinSafeInterval.Seconds());
    }

    float TRpsFilter::GetRpsById(const TUid& id) const {
        TReadGuard guard(Mutex);

        auto it = Map.find(id);
        if (it != Map.end()) {
            return CalcRps(it->second);
        }
        return 0;
    }

    void TRpsFilter::DecrementEntries() {
        for (auto it = Map.begin(); it != Map.end();) {
            it->second.DecrementRequestsCount(RememberFor);
            if (it->second.GetRequests() == 0) {
                Map.erase(it++);
            } else {
                it++;
            }
        }
    }

    void TRpsFilter::RecalcUser(const NAntiRobot::TUid& id, TInstant requestTime) {
        TWriteGuard guard(Mutex);

        TRpsFilter::LastTime = requestTime;
        auto it = Map.find(id);
        if (it != Map.end()) {
            it->second.IncrementRequestsCount(RememberFor);
        } else if (Map.size() < MaxSizeAllowed) {
            auto [insertedIt, wasInserted] = Map.emplace(id, TRequestRecord(LastTime));
            Y_ASSERT(wasInserted);
        } else {
            DecrementEntries();
        }
    }
}

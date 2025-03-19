#pragma once
#include "one_restriction.h"

class TTimeRestrictionsPool {
protected:
    TVector<TTimeRestriction> Restrictions;
private:
    using TProto = NRTLineProto::TTimeRestrictionsPool;
    struct TRestrictionIterator {
        TInstant Time;
        TInstant TimeNext;
        bool ActualState;
        const TTimeRestriction* Restriction;
        bool operator< (const TRestrictionIterator& item) const {
            return TimeNext > item.TimeNext;
        }
    };

public:

    TProto SerializeToProto() const;

    Y_WARN_UNUSED_RESULT bool DeserializeFromProto(const TProto& info);

    const TVector<TTimeRestriction>& GetRestrictions() const {
        return Restrictions;
    }

    ui32 RestrictionsCount() const {
        return Restrictions.size();
    }

    const TTimeRestriction* GetWidestRestriction(const TInstant start, const TInstant finish) const;

    void Merge(const TTimeRestrictionsPool& pool) {
        for (auto&& i : pool.Restrictions) {
            Restrictions.push_back(i);
        }
    }

    NJson::TJsonValue SerializeToJson() const;

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& info);

    static NFrontend::TScheme GetScheme();

    TTimeRestrictionsPool& Add(const TTimeRestriction& restriction) {
        Restrictions.push_back(restriction);
        return *this;
    }
    bool Empty() const {
        return Restrictions.empty();
    }

    void Clear() {
        Restrictions.clear();
    }

    ui32 GetDurationToSwitch(const TInstant time) const;

    TDuration GetCrossSize(const TInstant from, const TInstant to) const;

    TVector<TTimeInterval> CrossInterval(const TInstant from, const TInstant to, const ui32 maxTicks = 40) const;

    bool IsIntervalIncluded(const TTimeInterval& interval) const {
        return IsActualNow(interval.GetFrom()) && GetNextCorrectionTime(interval.GetFrom()) >= interval.GetTo();
    }

    TMaybe<TTimeOfDayInterval> GetDayInterval(const TTimeRestriction::EWeekDay day) const;

    TMaybe<TTimeOfDayInterval> GetDayInterval(const TInstant dayInstant) const;

    bool IsIntervalsIncluded(const TVector<TTimeInterval>& intervals) const;

    TInstant GetNextCorrectionTime(const TInstant time, const TDuration timeout = TDuration::Days(10)) const;

    void SetTimezoneShift(const i16 shift);

    bool IsActualNow(TInstant timestamp) const;

};

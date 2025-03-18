#pragma once

#include "log.h"

#include <util/datetime/base.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <library/cpp/deprecated/atomic/atomic.h>
#include <util/system/yassert.h>

template<class T>
struct TCounterName {
    TCounterName(T id, const TString& name)
        : Id(id)
        , Name(name)
    {
    }

    T Id;
    TString Name;
};

template<class T>
class TCounters {
public:
    explicit TCounters(size_t size)
        : Size(size)
    {
        Counters = new TAtomic[size];
        for (size_t i = 0; i < size; i++) {
            Counters[i] = 0;
        }
    }

    TCounters(const TCounters& counter)
        : Size(counter.Size)
    {
        Counters = new TAtomic[counter.Size];
        for (size_t i = 0; i < counter.Size; i++) {
            Counters[i] = counter.Counters[i];
        }
    }

    ~TCounters() {
        delete[] Counters;
    }

    TAtomic& operator[](T id) {
        Y_ASSERT(size_t(id) < Size);
        return Counters[id];
    }

    const TAtomic& operator[](T id) const {
        Y_ASSERT(size_t(id) < Size);
        return Counters[id];
    }

private:
    const TCounters& operator=(const TCounters&);

    TAtomic* Counters;
    size_t Size;
};

template<class T>
class TTimeCounters {
public:
    typedef TVector<TCounterName<T> > TNamesVector;

private:
    typedef TVector<TCounters<T> > TVectorType;

    struct TReminders {
        size_t FiveSeconds;
        size_t Minutes;
        size_t TenMinutes;

        TReminders(size_t fiveSeconds, size_t minutes, size_t tenMinutes)
            : FiveSeconds(fiveSeconds)
            , Minutes(minutes)
            , TenMinutes(tenMinutes)
        {
        }

        static TReminders Cons(TInstant now, const TTimeCounters& counters) {
            size_t fiveSeconds = (now.Seconds() / 5) % counters.FiveSeconds.size();
            size_t minutes = now.Minutes() % counters.Minutes.size();
            size_t tenMinutes = (now.Minutes() / 10) % counters.TenMinutes.size();
            return TReminders(fiveSeconds, minutes, tenMinutes);
        }
    };

    struct TState {
        TState()
            : FiveSeconds(0)
            , Minutes(0)
            , TenMinutes(0)
        {
        }

        TAtomic FiveSeconds;
        TAtomic Minutes;
        TAtomic TenMinutes;

        void Update(TReminders reminders) {
            AtomicSet(FiveSeconds, reminders.FiveSeconds);
            AtomicSet(Minutes, reminders.Minutes);
            AtomicSet(TenMinutes, reminders.TenMinutes);
        }

        TReminders GetReminders() {
            return TReminders(AtomicGet(FiveSeconds), AtomicGet(Minutes), AtomicGet(TenMinutes));
        }
    };

    long GetSumOfN(const TVectorType& vector, T id, size_t nextToStartFrom, size_t n) const {
        Y_ASSERT(n <= vector.size());

        long size = vector.size(); // signed type is essential for calculating k in 'int k = ...' string
        long result = 0;
        for (size_t i = 1; i <= n; i++) {
            long j = (nextToStartFrom - i);
            int k = (size + (j % size)) % size; // (size + (x % size)) % size is positive even if x is negative
            result += vector[k][id];
        }

        return result;
    }

    long GetSumOfNFiveSeconds(T id, TInstant now, size_t n) const {
        TReminders reminders = TReminders::Cons(now, *this);
        return GetSumOfN(FiveSeconds, id, reminders.FiveSeconds, n);
    }

    long GetSumOfNMinutes(T id, TInstant now, size_t n) const {
        TReminders reminders = TReminders::Cons(now, *this);
        return GetSumOfN(Minutes, id, reminders.Minutes, n);
    }

public:
    TTimeCounters(const TNamesVector& names, size_t fiveSecondsCount = 7, size_t minutesCount = 6, size_t tenMinutesCount = 6)
        : Names(names)
        , FiveSeconds(fiveSecondsCount, TCounters<T>(names.size()))
        , Minutes(minutesCount, TCounters<T>(names.size()))
        , TenMinutes(tenMinutesCount, TCounters<T>(names.size()))
        , State()
    {
    }

    void Add(T id, unsigned long i) {
        TReminders reminders = State.GetReminders();

        AtomicAdd(FiveSeconds[reminders.FiveSeconds][id], i);
        AtomicAdd(Minutes[reminders.Minutes][id], i);
        AtomicAdd(TenMinutes[reminders.TenMinutes][id], i);
    }

    void AddOne(T id) {
        Add(id, 1);
    }

    void Maintain(TInstant time = TInstant::Now()) {
        TReminders reminders = TReminders::Cons(time, *this);

        State.Update(reminders);

        for (typename TNamesVector::const_iterator i = Names.begin(); i != Names.end(); i++) {
            AtomicSet(FiveSeconds[(reminders.FiveSeconds + 1) % FiveSeconds.size()][i->Id], 0);
            AtomicSet(Minutes[(reminders.Minutes + 1) % Minutes.size()][i->Id], 0);
            AtomicSet(TenMinutes[(reminders.TenMinutes + 1) % TenMinutes.size()][i->Id], 0);
        }
    }

    void Dump(IOutputStream& dest = Cout) {
        for (typename TNamesVector::const_iterator i = Names.begin(); i != Names.end(); i++) {
            for (size_t j = 0; j < FiveSeconds.size(); j++) {
                dest << "FiveSeconds[" << j << "][" << i->Name << "] = " << FiveSeconds[j][i->Id] << "; ";
            }
            dest << "\n";
            for (size_t j = 0; j < Minutes.size(); j++) {
                dest << "Minutes[" << j << "][" << i->Name << "] = " << Minutes[j][i->Id] << "; ";
            }
            dest << "\n";
            for (size_t j = 0; j < TenMinutes.size(); j++) {
                dest << "TenMinutes[" << j << "][" << i->Name << "] = " << TenMinutes[j][i->Id] << "; ";
            }
            dest << "\n";
        }
    }

    long Get5Seconds(T id, TInstant now) const {
        return GetSumOfNFiveSeconds(id, now, 1);
    }

    long Get15Seconds(T id, TInstant now) const {
        return GetSumOfNFiveSeconds(id, now, 3);
    }

    long Get30Seconds(T id, TInstant now) const {
        return GetSumOfNFiveSeconds(id, now, 6);
    }

    long GetOneMinute(T id, TInstant now) const {
        return GetSumOfNMinutes(id, now, 1);
    }

    long Get3Minutes(T id, TInstant now) const {
        return GetSumOfNMinutes(id, now, 3);
    }

    long Get5Minutes(T id, TInstant now) const {
        return GetSumOfNMinutes(id, now, 5);
    }

    long GetHalfAnHour(T id, TInstant now) const {
        TReminders reminders = TReminders::Cons(now, *this);
        return GetSumOfN(TenMinutes, id, reminders.TenMinutes, 3);
    }

    const TString& GetNameById(T id) const {
        return Names[id].Name;
    }

    const TNamesVector& GetNames() const {
        return Names;
    }

private:
    TNamesVector Names;

    TVectorType FiveSeconds;
    TVectorType Minutes;
    TVectorType TenMinutes;

    TState State;
};

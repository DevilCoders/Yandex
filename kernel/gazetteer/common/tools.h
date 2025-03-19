#pragma once

#include <util/datetime/cputimer.h>
#include <util/generic/string.h>
#include <util/string/util.h>
#include <util/string/cast.h>
#include <library/cpp/deprecated/atomic/atomic.h>


namespace NTools {


class TPerformanceCounterBase {
public:
    // @logFreq - print log message once per @log_frequency items processed.
    TPerformanceCounterBase(const TString& name, size_t logFreq, bool suspend);
    virtual ~TPerformanceCounterBase()
    {
    }

    inline void operator++() {
        Increase(1);
    }

    inline void operator+=(size_t count) {
        Increase(count);
    }

    inline void Suspend() {
        LastSuspend = Now();
    }

    inline void IncSuspend(size_t count = 1) {
        Increase(count);
        LastSuspend = Now();
    }

    inline void Resume() {
        // treat as if we just started later
        TDuration pause = Now() - LastSuspend;
        StartTime += pause;
        PrevTime += pause;

        LastSuspend = TInstant::Zero();
    }

    inline void SetOutputStream(IOutputStream& out) {
        Output = &out;
    }

    void Print(size_t currentCounter) const;

protected:
    virtual size_t DoIncrease(size_t count) = 0; // returns new counter value if need to print counter, otherwise 0

    void Increase(size_t count) {
        if (size_t newCounter = DoIncrease(count)) {
            Print(newCounter);
            PrevTime = Now();
        }
    }

    virtual void DoPrint(bool finished, size_t currentCounter) const;

    virtual TString CustomInfo() const {
        return TString();
    }

    void PrintCustomInfo() const;

    void Finish() const;

    virtual size_t GetCurrentCounter() const = 0;

protected:
    TString Name;

    size_t LogFreq;

    TInstant StartTime;
    TInstant PrevTime;
    TInstant LastSuspend;

    IOutputStream* Output;
};

class TPerformanceCounter: public TPerformanceCounterBase {
public:
    TPerformanceCounter(const TString& name = "(counter)", size_t logFreq = 100000, bool suspend = false)
        : TPerformanceCounterBase(name, logFreq, suspend)
        , Counter(0)
        , NextCount(logFreq)
    {
    }

    ~TPerformanceCounter() override {
        Finish();
    }

    size_t DoIncrease(size_t count) override {
        Counter += count;
        if (Y_UNLIKELY(Counter >= NextCount)) {
            NextCount += LogFreq;
            return Counter;
        } else
            return 0;
    }

    size_t GetCurrentCounter() const override {
        return Counter;
    }

protected:
    size_t Counter;
    size_t NextCount;
};

class TAtomicPerformanceCounter: public TPerformanceCounterBase {
public:
    TAtomicPerformanceCounter(const TString& name = "(counter)", size_t logFreq = 100000, bool suspend = false)
        : TPerformanceCounterBase(name, logFreq, suspend)
        , Counter(0)
    {
    }

    ~TAtomicPerformanceCounter() override {
        Finish();
    }

protected:
    size_t DoIncrease(size_t count) override {
        const size_t addedCounter = static_cast<size_t>(AtomicAdd(Counter, count));
        const size_t prevCounter = addedCounter - count;
        if (addedCounter / LogFreq != prevCounter / LogFreq)
            return addedCounter;
        else
            return 0;
    }

    size_t GetCurrentCounter() const override {
        return static_cast<size_t>(AtomicGet(Counter));
    }

protected:
    TAtomic Counter;
};

class TBytePerformanceCounter: public TPerformanceCounter {
public:
    TBytePerformanceCounter(const TString& name, size_t logFreqBytes = 1024*1024*1024, bool suspend = false)
        : TPerformanceCounter(name, logFreqBytes, suspend)
    {
    }

    ~TBytePerformanceCounter() override {
        Finish();
    }

protected:
    void DoPrint(bool finished, size_t currentCounter) const override;
};


} //namespace NTools

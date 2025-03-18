#pragma once

#include <util/generic/vector.h>
#include <util/generic/string.h>
#include <util/generic/noncopyable.h>
#include <util/system/defaults.h>
#include <util/system/yassert.h>

template <class T>
class TBigrammer: private TNonCopyable {
public:
    typedef T TValue;
    typedef TVector<TValue> TOut;

    static ui32 GetVersion() {
        return 1;
    }
    static TString GetDescription() {
        return "shift-xor based bigrammer";
    }

public:
    TBigrammer(TOut* Output)
        : output(Output)
        , prevValue()
        , hasPrevValue(false)
    {
        Y_VERIFY((output != nullptr), "output == NULL");
    }

public:
    void OnStart() {
        output->clear();
        hasPrevValue = false;
    }

    void OnValue(const T Value) {
        if (hasPrevValue) {
            output->push_back((prevValue << 2) ^ Value);
        } else {
            output->push_back(Value);
        }
        prevValue = Value;
        hasPrevValue = true;
    }

    template <class TIt>
    void OnValues(TIt Begin, TIt End) {
        while (Begin != End) {
            OnValue(*Begin);
            ++Begin;
        }
    }

    void Break() {
        if (hasPrevValue) {
            output->push_back(prevValue);
            hasPrevValue = false;
        }
    }

    void OnEnd() {
        Break();
    }

private:
    TOut* output;

    T prevValue;
    bool hasPrevValue;
};

template <class T>
class TUnigrammer: private TNonCopyable {
public:
    typedef T TValue;
    typedef TVector<TValue> TOut;

    static ui32 GetVersion() {
        return 2;
    }
    static TString GetDescription() {
        return "unigrammer";
    }

public:
    TUnigrammer(TOut* Output)
        : output(Output)
    {
        Y_VERIFY((output != nullptr), "output == NULL");
    }

public:
    void OnStart() {
        output->clear();
    }

    void OnValue(const T Value) {
        output->push_back(Value);
    }

    template <class TIt>
    void OnValues(TIt Begin, TIt End) {
        while (Begin != End) {
            OnValue(*Begin);
            ++Begin;
        }
    }

    void Break() {
    }

    void OnEnd() {
        Break();
    }

private:
    TOut* output;
};

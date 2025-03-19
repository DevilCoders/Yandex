#pragma once

#include <library/cpp/unistat/unistat.h>
#include <search/common_signals/signals.h>

#include <util/generic/map.h>

class TSaasUnistatSignals : public NSearch::TCommonSearchSignals {
public:
    NUnistat::TPriority Prio(const TString& code = Default<TString>(), bool isService = false, bool isExtraInfo = false) const;
    static void PushSignal(TUnistat& inst, const TString& signal, double value);
    static void PushSignalWithCode(TUnistat& inst, const TString& signal, const TString& code, const TString& xxCode);

    inline static void PushGlobalSignal(const TString& signal, double value) {
        PushSignal(TUnistat::Instance(), signal, value);
    }
    inline static void PushGlobalSignalWithCode(const TString& signal, const TString& code, const TString& xxCode) {
        PushSignalWithCode(TUnistat::Instance(), signal, code, xxCode);
    }
    template <class T>
    inline static void PushGlobalSignal(const T& signals, double value) {
        for (auto&& signal : signals) {
            PushGlobalSignal(signal, value);
        }
    }
    template <class T>
    inline static void PushGlobalSignalWithCode(const T& signals, const TString& code, const TString& xxCode) {
        for (auto&& signal : signals) {
            PushGlobalSignalWithCode(signal, code, xxCode);
        }
    }
};

class TSaasProxyUnistatSignals : public TSaasUnistatSignals {
public:
    virtual void AddCode(const int code);
    virtual void AddService(const TString& service);

protected:
    TSet<TString> Services;
    TSet<TString> Codes;
};

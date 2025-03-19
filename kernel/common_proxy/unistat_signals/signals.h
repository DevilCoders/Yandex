#pragma once

#include <library/cpp/unistat/unistat.h>
#include <library/cpp/yconf/conf.h>
#include <util/generic/string.h>


class IUnistatSignalSource {
public:
    virtual ~IUnistatSignalSource() {}
    virtual void RegisterSignals(TUnistat& tass) const = 0;
};

class TUnistatSignalsConfig {
public:
    inline const TString& GetSignalPrefix() const {
        return SignalPrefix;
    }
    void ToString(IOutputStream& so) const;
    void Init(const TYandexConfig::Section& section);
private:
    TString SignalPrefix = "cproxy-CTYPE-";
};

class TCommonProxySignals  {
public:
    using TProcessors = TVector<IUnistatSignalSource*>;

    void Init(TUnistat& t) const;

    static void BuildSignals(const TProcessors& processors, const TUnistatSignalsConfig& config);

    static TString GetSignalName(const TString& processor, const TString& signalName);
    static void PushSpecialSignal(const TString& processor, const TString& signalName, double value);
    static void PushAliveSignal(bool alive);
    static const NUnistat::TIntervals TimeIntervals;

private:
    TCommonProxySignals(const TProcessors& processors);
    static const TUnistatSignalsConfig*& MutableConfigPtr();
    const TProcessors& Processors;
};

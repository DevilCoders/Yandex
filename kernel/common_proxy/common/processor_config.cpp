#include "processor_config.h"

namespace NCommonProxy {


    ui32 TProcessorConfig::GetThreads() const {
        return Threads;
    }

    ui32 TProcessorConfig::GetMaxQueueSize() const {
        return MaxQueueSize;
    }

    const TString& TProcessorConfig::GetType() const {
        return Type;
    }

    bool TProcessorConfig::IsIgnoredSignal(const TString& signal) const {
        return IgnoredSignals.contains(signal);
    }

    void TProcessorConfig::DoToString(IOutputStream& so) const {
        so << "Type: " << Type << Endl;
        so << "Threads: " << Threads << Endl;
        so << "MaxQueue: " << MaxQueueSize << Endl;
        so << "Ignored signals: ";
        for (const auto& signal : IgnoredSignals) {
            so << signal << " ";
        }
        so << Endl;
    }

    void TProcessorConfig::DoInit(const TYandexConfig::Section& componentSection) {
        const TYandexConfig::Directives& dir = componentSection.GetDirectives();
        Threads = dir.Value("Threads", 0);
        MaxQueueSize = dir.Value("MaxQueue", 100 * Threads);

        TString IgnoredSignalsStr;
        if (dir.GetValue("IgnoredSignals", IgnoredSignalsStr)) {
            TVector<TString> split;
            Split(IgnoredSignalsStr, ",", split);
            IgnoredSignals.insert(split.begin(), split.end());
        }
    }

    bool TProcessorConfig::DoCheck() const {
        return true;
    }
}

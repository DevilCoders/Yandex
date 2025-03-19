#include "link_config.h"

namespace NCommonProxy {

    void TLinkConfig::Init(const TYandexConfig::Section& section) {
        const auto& dir = section.GetDirectives();
        dir.GetValue("From", From);
        dir.GetValue("To", To);
        dir.GetValue("Enabled", Enabled);
        dir.GetValue("RpsLimit", RpsLimit);

        TString IgnoredSignalsStr;
        if (dir.GetValue("IgnoredSignals", IgnoredSignalsStr)) {
            TVector<TString> split;
            Split(IgnoredSignalsStr, ",", split);
            IgnoredSignals.insert(split.begin(), split.end());
        }
    }

    bool TLinkConfig::IsIgnoredSignal(const TString& signal) const {
        return IgnoredSignals.contains(signal);
    }

    void TLinkConfig::ToString(TStringOutput& so) const {
        so << "<Link>" << Endl;
        so << "From:" << From << Endl;
        so << "To:" << To << Endl;
        so << "Enabled:" << Enabled << Endl;
        so << "RpsLimit:" << RpsLimit << Endl;
        so << "Ignored signals: ";
        for (const auto& signal : IgnoredSignals) {
            so << signal << " ";
        }
        so << "</Link>" << Endl;
    }

}

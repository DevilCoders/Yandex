#include "limited_source.h"

namespace NCommonProxy {
    void TLimitedSource::TConfig::TAlert::Init(const TYandexConfig::Section& section) {
        Period = section.GetDirectives().Value("Period", TDuration::Zero());
        if (!section.GetDirectives().GetValue("SuccessRatio", SuccessRatio))
            ythrow yexception() << "SuccessRatio not set";
    }

    void TLimitedSource::TConfig::TAlert::ToString(IOutputStream& so) const {
        so << "<Alert>" << Endl;
        so << "SuccessRatio:" << SuccessRatio << Endl;
        so << "Period:" << Period.ToString() << Endl;
        so << "</Alert>" << Endl;
    }

    bool TLimitedSource::TConfig::DoCheck() const {
        return true;
    }

    void TLimitedSource::TConfig::DoInit(const TYandexConfig::Section& componentSection) {
        TSource::TConfig::DoInit(componentSection);
        CheckAlertsPeriod = componentSection.GetDirectives().Value("CheckAlertsPeriod", 1000);
        TYandexConfig::TSectionsMap children = componentSection.GetAllChildren();
        for (auto range = children.equal_range("Alert"); range.first != range.second; ++range.first) {
            Alerts.push_back(TAlert());
            Alerts.back().Init(*range.first->second);
        }
    }

    void TLimitedSource::TConfig::DoToString(IOutputStream& so) const {
        TSource::TConfig::DoToString(so);
        so << "CheckAlertsPeriod: " << CheckAlertsPeriod << Endl;
        for (const auto& alert : Alerts) {
            alert.ToString(so);
        }
    }
}

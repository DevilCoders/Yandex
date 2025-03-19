#include "config.h"

namespace NCommonProxy {

    TServerConfig::TServerConfig(const TServerConfigConstructorParams& params)
        : Daemon(*params.GetDaemonConfig())
        , ProccessorsConfigs("Processors")
    {
        TAnyYandexConfig yandConf;
        if (!yandConf.ParseMemory(params.GetText().data())) {
            TString err;
            yandConf.PrintErrors(err);
            FAIL_LOG("%s", err.data());
        }
        const TYandexConfig::Section* appSection = yandConf.GetFirstChild("Proxy");
        VERIFY_WITH_LOG(appSection, "there is no Proxy section");
        TYandexConfig::TSectionsMap sections = appSection->GetAllChildren();
        ProccessorsConfigs.Init(*this, sections);
        auto i = sections.find("Links");
        if (i != sections.end()) {
            TSet<TString> modules = ProccessorsConfigs.GetModules();
            TYandexConfig::TSectionsMap links = i->second->GetAllChildren();
            for (auto range = links.equal_range("Link"); range.first != range.second; ++range.first) {
                Links.emplace_back();
                Links.back().Init(*range.first->second);
                VERIFY_WITH_LOG(modules.contains(Links.back().From) && modules.contains(Links.back().To), "invalid link from %s to %s", Links.back().From.data(), Links.back().To.data());
            }
        }

        i = sections.find("UnistatSignals");
        if (i != sections.end()) {
            UnistatSignals.Init(*i->second);
        }
        TDuration::TryParse(appSection->GetDirectives().Value<TString>("StartTimeout"), StartTimeout);
    }

    const TDaemonConfig& TServerConfig::GetDaemonConfig() const {
        return Daemon;
    }

    TSet<TString> TServerConfig::GetModulesSet() const {
        return TSet<TString>();
    }

    TString TServerConfig::ToString() const {
        TString s;
        TStringOutput so(s);
        so << Daemon.ToString("DaemonConfig");
        so << "<Proxy>" << Endl;
        ProccessorsConfigs.ToString(&so);
        so << "<Links>" << Endl;
        for (const auto& link : Links) {
            link.ToString(so);
        }
        so << "</Links>" << Endl;
        so << "<UnistatSignals>" << Endl;
        UnistatSignals.ToString(so);
        so << "</UnistatSignals>" << Endl;
        so << "StartTimeout: " << StartTimeout.ToString() << Endl;
        so << "</Proxy>" << Endl;
        return s;
    }

}

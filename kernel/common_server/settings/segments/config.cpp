#include "config.h"
#include "segments.h"

namespace NCS {

    TSettingsPackConfig::TFactory::TRegistrator<TSettingsPackConfig> TSettingsPackConfig::Registrator(TSettingsPackConfig::GetTypeName());

    void TSettingsPackConfig::DoInit(const TYandexConfig::Section& section) {
        auto sections = section.GetAllChildren();
        auto it = sections.find("Segments");
        AssertCorrectConfig(it != sections.end(), "no segments section for settings config");
        {
            auto segmentSections = it->second->GetAllChildren();
            bool hasDefault = false;
            for (auto&& i : segmentSections) {
                TSettingsConfigContainer configContainer;
                configContainer.Init(i.second);
                AssertCorrectConfig(Configs.emplace(i.first, std::move(configContainer)).second, "settings section ids duplication");
                if (i.first == "default") {
                    hasDefault = true;
                }
            }
            AssertCorrectConfig(hasDefault, "No 'default' settings section");
        }
    }

    void TSettingsPackConfig::DoToString(IOutputStream& os) const {
        os << "<Segments>" << Endl;
        for (auto&& i : Configs) {
            os << "<" << i.first << ">";
            i.second->ToString(os);
            os << "</" << i.first << ">";
        }
        os << "</Segments>" << Endl;
    }

    ISettings::TPtr TSettingsPackConfig::Construct(const IBaseServer& server) const {
        auto result = MakeHolder<TSegmentedSettings>(*this);
        for (auto&& i : Configs) {
            auto segment = i.second->Construct(server);
            AssertCorrectConfig(!!segment, "Incorrect segment in settings: %s", i.first.data());
            result->RegisterSegment(i.first, segment);
        }
        CHECK_WITH_LOG(result->Start()) << "Cannot start settings pack object" << Endl;
        return result.Release();
    }

}

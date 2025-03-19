#include "config.h"

namespace NServiceMonitor {

    void TServerConfig::Init(const TYandexConfig::Section* section) {
        TBase::Init(section);
        auto children = section->GetAllChildren();
        {
            auto it = children.find("YpAPI");
            VERIFY_WITH_LOG(it != children.end(), "Not found required config section \"YpAPI\".");
            YpApiConfig.Init(it->second);
        }
        {
            auto it = children.find("ServerInfoStorage");
            VERIFY_WITH_LOG(it != children.end(), "Not found required config section \"ServerInfoStorage\".");
            ServerInfoStorageConfig.Init(it->second);
        }
    }

    void TServerConfig::DoToString(IOutputStream& os) const {
        os << "<YpAPI>" << Endl;
        YpApiConfig.ToString(os);
        os << "</YpAPI>" << Endl;

        os << "<ServerInfoStorage>" << Endl;
        ServerInfoStorageConfig.ToString(os);
        os << "</ServerInfoStorage>" << Endl;
    }

}

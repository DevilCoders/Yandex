#include "config.h"

namespace NCS {

    void TSnapshotsManagerConfig::Init(TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("HistoryConfig");
        if (it != children.end()) {
            HistoryConfig.Init(it->second);
        }
    }

    void TSnapshotsManagerConfig::ToString(IOutputStream& os) const {
        os << "<HistoryConfig>" << Endl;
        HistoryConfig.ToString(os);
        os << "</HistoryConfig>" << Endl;
    }

    void TSnapshotGroupsManagerConfig::Init(TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        auto it = children.find("HistoryConfig");
        if (it != children.end()) {
            HistoryConfig.Init(it->second);
        }
    }

    void TSnapshotGroupsManagerConfig::ToString(IOutputStream& os) const {
        os << "<HistoryConfig>" << Endl;
        HistoryConfig.ToString(os);
        os << "</HistoryConfig>" << Endl;
    }

    void TSnapshotsControllerConfig::Init(const TYandexConfig::Section* section) {
        auto children = section->GetAllChildren();
        DBName = section->GetDirectives().Value("DBName", DBName);
        {
            auto it = children.find("SnapshotsManager");
            if (it != children.end()) {
                SnapshotsManagerConfig.Init(it->second);
            }
        }
        {
            auto it = children.find("SnapshotGroupsManager");
            if (it != children.end()) {
                SnapshotGroupsManagerConfig.Init(it->second);
            }
        }
    }

    void TSnapshotsControllerConfig::ToString(IOutputStream& os) const {
        os << "<SnapshotsManager>" << Endl;
        SnapshotsManagerConfig.ToString(os);
        os << "</SnapshotsManager>" << Endl;
        os << "<SnapshotGroupsManager>" << Endl;
        SnapshotGroupsManagerConfig.ToString(os);
        os << "</SnapshotGroupsManager>" << Endl;
    }

}

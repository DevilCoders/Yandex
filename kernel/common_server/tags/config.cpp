#include "config.h"

void TTagsManagerConfig::Init(const TYandexConfig::Section* section) {
    auto children = section->GetAllChildren();
    {
        auto it = children.find("HistoryConfig");
        if (it != children.end()) {
            HistoryConfig.Init(it->second);
        }
    }
}

void TTagsManagerConfig::ToString(IOutputStream& os) const {
    os << "<HistoryConfig>" << Endl;
    HistoryConfig.ToString(os);
    os << "</HistoryConfig>" << Endl;
}

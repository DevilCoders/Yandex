#pragma once
#include <kernel/common_server/api/history/config.h>
#include <library/cpp/yconf/conf.h>
#include <kernel/common_server/common/manager_config.h>

using TTagDescriptionsManagerConfig = NCS::NCommon::TMetaManagerConfig;

class TTagsManagerConfig {
private:
    CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
public:
    TTagsManagerConfig() {

    }

    void Init(const TYandexConfig::Section* section);
    void ToString(IOutputStream& os) const;
};

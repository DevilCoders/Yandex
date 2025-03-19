#pragma once
#include <kernel/common_server/api/history/config.h>
#include <library/cpp/yconf/conf.h>

namespace NCS {
    class TSnapshotsManagerConfig {
    private:
        CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
    public:
        TSnapshotsManagerConfig() = default;
        void Init(TYandexConfig::Section* section);
        void ToString(IOutputStream& os) const;
    };

    class TSnapshotGroupsManagerConfig {
    private:
        CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
    public:
        TSnapshotGroupsManagerConfig() = default;
        void Init(TYandexConfig::Section * section);
        void ToString(IOutputStream & os) const;
    };

    class TSnapshotsControllerConfig {
    private:
        CSA_READONLY_DEF(TString, DBName);
        CSA_READONLY_DEF(TSnapshotsManagerConfig, SnapshotsManagerConfig);
        CSA_READONLY_DEF(TSnapshotGroupsManagerConfig, SnapshotGroupsManagerConfig);
    public:
        TSnapshotsControllerConfig() = default;
        void Init(const TYandexConfig::Section* section);

        void ToString(IOutputStream& os) const;
    };
}

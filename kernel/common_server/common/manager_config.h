#pragma once
#include <kernel/common_server/api/history/config.h>
#include <library/cpp/yconf/conf.h>
#include <library/cpp/mediator/global_notifications/system_status.h>

namespace NCS {
    namespace NCommon {
        template <bool IsMeta = false>
        class TManagerConfigImpl {
        private:
            CSA_READONLY_DEF(THistoryConfig, HistoryConfig);
            CSA_READONLY_DEF(TString, DBName);
            CSA_FLAG(TManagerConfigImpl, MetaInfo, IsMeta);
        public:
            TManagerConfigImpl() = default;
            virtual ~TManagerConfigImpl() = default;
            virtual void Init(const TYandexConfig::Section* section) {
                auto children = section->GetAllChildren();
                auto it = children.find("HistoryConfig");
                if (it != children.end()) {
                    HistoryConfig.Init(it->second);
                }
                MetaInfoFlag = section->GetDirectives().Value("IsMetaInfo", MetaInfoFlag);
                DBName = section->GetDirectives().Value("DBName", DBName);
                AssertCorrectConfig(!!DBName, "empty DBName field");
                if (IsMetaInfo()) {
                    HistoryConfig.NoLimitsHistory();
                }
            }

            virtual void ToString(IOutputStream& os) const {
                os << "<HistoryConfig>" << Endl;
                HistoryConfig.ToString(os);
                os << "</HistoryConfig>" << Endl;
                os << "DBName: " << DBName << Endl;
                os << "IsMetaInfo: " << IsMetaInfo() << Endl;
            }
        };

        using TManagerConfig = TManagerConfigImpl<false>;
        using TMetaManagerConfig = TManagerConfigImpl<true>;
    }
} // namespace NCSFintech

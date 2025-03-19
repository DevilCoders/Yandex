#include "backend_creator_helpers.h"

#include <library/cpp/yconf/patcher/unstrict_config.h>
#include <library/cpp/logger/init_context/yconf.h>
#include <library/cpp/logger/uninitialized_creator.h>

namespace NCommonServer::NUtil {

    THolder<ILogBackendCreator> CreateLogBackendCreator(const TYandexConfig::Section& section, const TString& subSecName, const TStringBuf dirName, const TStringBuf defaultLog) {
        auto children = section.GetAllChildren();
        auto it = children.find(subSecName);
        if (it != children.end()) {
            return ILogBackendCreator::Create(TLogBackendCreatorInitContextYConf(*it->second));
        };
        if (dirName || defaultLog) {
            auto b = MakeHolder<TLogBackendCreatorUninitialized>();
            TString log = TString(defaultLog);
            if (dirName) {
                section.GetDirectives().GetValue(dirName, log);
            }
            b->InitCustom(log, LOG_MAX_PRIORITY, false);
            return std::move(b);
        }
        return THolder<ILogBackendCreator>();
    }

    void LogBackendCreatorToString(const THolder<ILogBackendCreator>& backend, const TStringBuf sectionName, IOutputStream& os) {
        if (!backend) {
            return;
        }
        if (sectionName) {
            os << "<" << sectionName << ">" << Endl;
        }
        {
            TUnstrictConfig cfg;
            Y_VERIFY(cfg.ParseJson(backend->AsJson()));
            SectionToStream(cfg.GetRootSection(), os);
        }
        if (sectionName) {
            os << "</" << sectionName << ">" << Endl;
        }
    }

}

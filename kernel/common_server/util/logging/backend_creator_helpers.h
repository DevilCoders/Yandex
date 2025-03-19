#pragma once

#include <library/cpp/logger/backend_creator.h>
#include <library/cpp/yconf/conf.h>
#include <util/generic/strbuf.h>
#include <util/stream/output.h>

namespace NCommonServer::NUtil {

    THolder<ILogBackendCreator> CreateLogBackendCreator(const TYandexConfig::Section& section, const TString& subSecName,
        const TStringBuf dirName = TStringBuf(), const TStringBuf defaultLog = TStringBuf());

    void LogBackendCreatorToString(const THolder<ILogBackendCreator>& backend, const TStringBuf sectionName, IOutputStream& os);
}

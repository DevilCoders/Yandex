#pragma once

#include "qd_constants.h"
#include "qd_source_updater.h"

#include <library/cpp/scheme/scheme.h>

#include <util/folder/path.h>
#include <util/generic/strbuf.h>

namespace NQueryData {

    TFsPath ResolveSymlinks(TFsPath path, ui32 level = 0);

    void ReportError(NSc::TValue& vv, TStringBuf error);

    void ReportNullUpdater(NSc::TValue& v, const char* which, ui32 idx, TString key = TString());

    void ReportNullSource(NSc::TValue& v);

    void ReportRealFile(NSc::TValue& vv, const TFsPath& path);

    TString PrintId(TStringBuf path);

    NSc::TValue NextReportNode(NSc::TValue& parent, TString id);

    void ReportSourceStats(NSc::TValue& v, EStatsVerbosity sv, const TAbstractSourceUpdater* p);

    void ReportRealSourceStats(NSc::TValue& v, EStatsVerbosity sv, const TSourceChecker* p);

}

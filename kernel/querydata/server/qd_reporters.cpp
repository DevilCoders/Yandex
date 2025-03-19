#include "qd_reporters.h"

#include <util/generic/yexception.h>

namespace NQueryData {

    TFsPath ResolveSymlinks(TFsPath path, ui32 level) {
        path.Fix();
        if (level > 100)
            ythrow yexception() << "probably link recursion";

        if (path.IsSymlink()) {
            path = path.ReadLink();
        }

        if (path.Parent().GetPath() == "." || path.Parent().GetPath() == "/" || path.Parent().GetPath().empty())
            return path;

        return ResolveSymlinks(path.Parent(), level + 1) / path.GetName();
    }


    void ReportError(NSc::TValue& vv, TStringBuf error) {
        vv["errors"].Push(error);
    }

    void ReportNullUpdater(NSc::TValue& v, const char* which, ui32 idx, TString key) {
        ReportError(v, Sprintf("null %s updater at index %u %s", which, idx, key.data()));
    }

    void ReportNullSource(NSc::TValue& v) {
        ReportError(v, "no updater for source");
    }

    void ReportRealFile(NSc::TValue& vv, const TFsPath& path) {
        if (!path.Exists()) {
            ReportError(vv, "doesn't exist");
        } else if (!path.IsFile()) {
            ReportError(vv, "not a file");
        }

        vv["file"]["path"] = path.GetPath();
        vv["file"]["real"] = ResolveSymlinks(path).GetPath();

        TFileStat stat;
        if (path.Stat(stat)) {
            vv["file"]["timestamp"] = stat.MTime;
            vv["file"]["size"] = stat.Size;
        }
    }

    TString PrintId(TStringBuf path) {
        if (!path) {
            return "NULL";
        }

        return TFsPath(path).GetName();
    }

    NSc::TValue NextReportNode(NSc::TValue& parent, TString id) {
        return parent.GetOrAdd(PrintId(id)).Push();
    }

    void ReportSourceStats(NSc::TValue& v, EStatsVerbosity sv, const TAbstractSourceUpdater* p) {
        TAbstractSourceUpdater::TSharedObjectPtr pp = p->GetObject();

         if (!!pp) {
            v.MergeUpdate(pp->GetStats(sv));
        } else {
            ReportError(v, "source is not instantiated");
        }
    }

    void ReportRealSourceStats(NSc::TValue& v, EStatsVerbosity sv, const TSourceChecker* p) {
        try {
            ReportRealFile(v, p->GetMonitorFileName());
        } catch (const yexception& e) {
            ReportError(v, e.what());
        }

        ReportSourceStats(v, sv, p);
    }

}

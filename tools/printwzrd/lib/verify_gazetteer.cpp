#include "verify_gazetteer.h"

#include <kernel/gazetteer/gazetteer.h>

#include <search/wizard/config/config.h>

#include <util/stream/file.h>
#include <util/folder/dirut.h>

namespace NPrintWzrd {

     bool FindArcadiaTop(const TWizardConfig& cfg, TFsPath& arcadiaTop, TFsPath& arcadia) {
        TFsPath workDir(cfg.GetWorkDir());
        const TString arcadiaName = "arcadia";
        const TString arcadiaDataName = "arcadia_tests_data";
        if (workDir.GetName() == arcadiaName || workDir.GetName() == arcadiaDataName)
            arcadiaTop = workDir.Parent();
        else
            return false;

        arcadia = arcadiaTop / arcadiaName;
        TFsPath arcadiaTestsData = arcadiaTop / arcadiaDataName;
        if (!arcadia.Exists() || !arcadiaTestsData.Exists())
            return false; // arcadia is not located in same dir as arcadia_tests_data
        return true;
    }

    bool VerifyGzt(const TString& gztBin, const TString& importPaths) {
        if (gztBin.EndsWith(".bin")) {                                   // main.gzt.bin
            const TString& gztSrc = gztBin.substr(0, gztBin.size() - 4);    // main.gzt
            if (gztSrc.EndsWith(".gzt")) {
                NGzt::VerifyGazetteerBinary(gztSrc, gztBin, importPaths);
                return true;
            }
        }
        Cerr << "Cannot verify " << gztBin << Endl;
        return false;   // cannot verify, skip
    }

    void VerifyGazetteerBinaries(const TWizardConfig& cfg) {
        TFsPath arcadiaTop;
        TFsPath arcadia;
        if (FindArcadiaTop(cfg, arcadiaTop, arcadia)) {
            TString importPaths = arcadiaTop.GetPath() + ":" + arcadia.GetPath();
            VerifyGzt(cfg.GetGazetteerDictionary(), importPaths);              // main.gzt.bin
        }
    }
}

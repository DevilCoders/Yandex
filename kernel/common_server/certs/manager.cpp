#include "manager.h"

#include <library/cpp/neh/https.h>

namespace NCS {
    bool TCertsManger::Start() noexcept {
        if (!!Config.GetCAPath()) {
            SetNehCaDir(Config.GetCAPath().GetPath());
        }
        if (!!Config.GetCAFile()) {
            SetNehCaFile(Config.GetCAFile().GetPath());
        }
        return true;
    }

    bool TCertsManger::Stop() noexcept {
        return true;
    }

    void TCertsManger::SetNehCaFile(const TString& path) const {
        NNeh::THttpsOptions::CAFile = path;
    }

    void TCertsManger::SetNehCaDir(const TString& path) const {
        NNeh::THttpsOptions::CAPath = path;
    }
}

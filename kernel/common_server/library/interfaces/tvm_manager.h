#pragma once

#include <library/cpp/tvmauth/client/facade.h>
#include <util/generic/ptr.h>

class TTvmConfig;

namespace NCS {
    class ITvmManager {
    public:
        virtual ~ITvmManager() = default;
        virtual TAtomicSharedPtr<NTvmAuth::TTvmClient> GetTvmClient(const TString& name) const = 0;
    };
}

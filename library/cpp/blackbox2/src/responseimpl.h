#pragma once

#include "xconfig.h"

#include <library/cpp/blackbox2/blackbox2.h>

#include <util/generic/ptr.h>

namespace NBlackbox2 {
    /// XML document wrapper
    class TResponse::TImpl {
    public:
        // init by full xml document
        TImpl(const TStringBuf response);
        // init by document part
        TImpl(const xmlConfig::Part* part);

        ~TImpl();

        bool GetIfExists(const TString& xpath, TString& variable);
        bool GetIfExists(const TString& xpath, long int& variable);
        xmlConfig::Parts GetParts(const TString& xpath);

    private:
        THolder<xmlConfig::XConfig> Xmlroot_;
        THolder<xmlConfig::Part> Doc_;
    };
}

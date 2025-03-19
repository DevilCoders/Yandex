#pragma once
#include "preparer_base.h"

namespace NStringMatchTracker {

    class TTranslitPreparer : public IPreparer {
    public:
        TString Prepare(const TTextWithDescription& descr) const override;
    };
}

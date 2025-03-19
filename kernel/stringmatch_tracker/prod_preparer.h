#pragma once
#include "preparer_base.h"

namespace NStringMatchTracker {

    class TProductionPreparer : public IPreparer {
        public:
            TString Prepare(const TTextWithDescription& descr) const override;

        private:
            bool IsGoodYandexLetter(unsigned char ch) const;
        };
}

#pragma once
#include <kernel/common_server/ut/scripts/default_actions.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/ut/scripts/abstract.h>

namespace NCS {
    namespace NFallbackProxy {
        class TCheckFallbackQueue: public NServerTest::TWaitCommonConditionedActionImpl {
        private:
            using TBase = NServerTest::TWaitCommonConditionedActionImpl;
            CSA_MAYBE(TCheckFallbackQueue, ui32, ExpectedSize);

        protected:
            virtual bool Check(NServerTest::ITestContext& context) const override;

        public:
            using TBase::TBase;
            TCheckFallbackQueue& ExpectSize(const ui32 size) {
                ExpectedSize = size;
                return *this;
            }
        };
    }
}

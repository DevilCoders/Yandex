#include "aws.h"

#include <aws/core/Aws.h>

namespace NAws {

namespace {
    class TAwsSdkRAII {
        public:
            TAwsSdkRAII() {
                Aws::InitAPI(SdkOptions);
            }
            ~TAwsSdkRAII() {
                Aws::ShutdownAPI(SdkOptions);
            }

        private:
            Aws::SDKOptions SdkOptions;
    };
}

void EnsureAwsSdkInitialized() {
    static TAwsSdkRAII awsSdk;
}

} // namespace NAws

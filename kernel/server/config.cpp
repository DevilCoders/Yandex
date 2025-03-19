#include "config.h"

namespace NServer {

    TSharedItsConfigBlob SharedItsConfigBlob(new TRefCountBlobHolder());
    TSharedHttpServerConfig SharedHttpServerConfig(new TRefCountHttpServerConfig());

    void UpdateSharedItsConfigBlob(TSharedItsConfigBlob::TPtr& conf) {
        conf.Reset(SharedItsConfigBlob.AtomicLoad());
    }

} // namespace NServer

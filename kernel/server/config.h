#pragma once


#include <kernel/server/protos/serverconf.pb.h>
#include <library/cpp/threading/hot_swap/hot_swap.h>

#include <util/memory/blob.h>

namespace NServer {


    /*
     * Provides HttpServerConfig with atomic reference counting
     */
    class TRefCountHttpServerConfig: public NServer::THttpServerConfig, public TAtomicRefCount<TRefCountHttpServerConfig> {
    public:
        TRefCountHttpServerConfig() = default;

        TRefCountHttpServerConfig(const NServer::THttpServerConfig& config)
            : THttpServerConfig(config)
        {
        }
    };

    /*
     * We can not inherit from TBlob because private methods Ref() and Unref() already exist in TBlob
     */
    class TRefCountBlobHolder: public TAtomicRefCount<TRefCountBlobHolder> {
    public:
        TRefCountBlobHolder() = default;

        TRefCountBlobHolder(const TBlob& blob)
            : Blob(blob)
        {
        }

        const TBlob& GetBlob() const {
            return Blob;
        }

    private:
        TBlob Blob;
    };

    using TSharedItsConfigBlob = THotSwap<TRefCountBlobHolder>;
    using TSharedHttpServerConfig = THotSwap<TRefCountHttpServerConfig>;

    extern TSharedItsConfigBlob SharedItsConfigBlob;
    extern TSharedHttpServerConfig SharedHttpServerConfig;

    void UpdateSharedItsConfigBlob(TSharedItsConfigBlob::TPtr& conf);

} // namespace NServer

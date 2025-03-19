#include "rdma.h"

#include "spdk.h"

#include <util/system/align.h>

namespace NCloud::NBlockStore::NSpdk {

namespace {

struct spdk_log_flag SPDK_LOG_rdma;

////////////////////////////////////////////////////////////////////////////////

int RegisterMemory(void* addr, size_t len, void* cookie)
{
    Y_UNUSED(cookie);

    int ret = spdk_mem_register(addr, len);
    if (ret) {
        SPDK_ERRLOG("unable to register memory region %p-%p: %s", addr,
            static_cast<char*>(addr) + len, strerror(-ret));
        return ret;
    }
    SPDK_INFOLOG(rdma, "register memory region %p-%p", addr,
        static_cast<char*>(addr) + len);

    return ret;
}

int UnregisterMemory(void* addr, size_t len, void* cookie)
{
    Y_UNUSED(cookie);

    int ret = spdk_mem_unregister(addr, len);
    if (ret) {
        SPDK_ERRLOG("unable to unregister memory region %p-%p: %s", addr,
            static_cast<char*>(addr) + len, strerror(-ret));
    } else {
        SPDK_INFOLOG(rdma, "unregister memory region %p-%p", addr,
            static_cast<char*>(addr) + len);
    }

    return ret;
}

}   // namespace

////////////////////////////////////////////////////////////////////////////////

NRdma::TRdmaHandler RdmaHandler()
{
    return NRdma::TRdmaHandler {
        RegisterMemory,
        UnregisterMemory,
    };
}

}   // namespace NCloud::NBlockStore::NSpdk

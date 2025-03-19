LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    address.cpp
    alloc.cpp
    config.cpp
    device.cpp
    device_proxy.cpp
    device_wrapper.cpp
    env.cpp
    env_stub.cpp
    env_test.cpp
    histogram.cpp
    rdma.cpp
    target.cpp
    target_iscsi.cpp
    target_nvmf.cpp
)

ADDINCL(
    contrib/libs/libaio
    contrib/libs/spdk/include
    contrib/libs/spdk/lib
    contrib/libs/spdk/module
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/diagnostics
    cloud/blockstore/libs/rdma

    cloud/storage/core/libs/diagnostics

    contrib/libs/spdk/lib/bdev
    contrib/libs/spdk/lib/conf
    contrib/libs/spdk/lib/env_dpdk
    contrib/libs/spdk/lib/event
    contrib/libs/spdk/lib/ioat
    contrib/libs/spdk/lib/iscsi
    contrib/libs/spdk/lib/json
    contrib/libs/spdk/lib/log
    contrib/libs/spdk/lib/net
    contrib/libs/spdk/lib/nvme
    contrib/libs/spdk/lib/nvmf
    contrib/libs/spdk/lib/rpc
    contrib/libs/spdk/lib/sock
    contrib/libs/spdk/lib/thread
    contrib/libs/spdk/lib/trace
    contrib/libs/spdk/lib/util

    contrib/libs/spdk/module/bdev/aio
    contrib/libs/spdk/module/bdev/iscsi
    contrib/libs/spdk/module/bdev/malloc
    contrib/libs/spdk/module/bdev/null
    contrib/libs/spdk/module/bdev/nvme
    contrib/libs/spdk/module/env_dpdk
    contrib/libs/spdk/module/event/subsystems/bdev
    contrib/libs/spdk/module/event/subsystems/iscsi
    contrib/libs/spdk/module/event/subsystems/net
    contrib/libs/spdk/module/event/subsystems/nvmf
    contrib/libs/spdk/module/sock/posix
)

END()

RECURSE_FOR_TESTS(qemu-ut)
RECURSE_FOR_TESTS(ut)

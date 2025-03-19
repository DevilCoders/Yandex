SRCS(
    cache.cpp
    config.cpp
    convert.cpp
    driver.cpp
    fs.cpp
    fs_impl.cpp
    fs_impl_attr.cpp
    fs_impl_data.cpp
    fs_impl_list.cpp
    fs_impl_lock.cpp
    fs_impl_node.cpp
    fs_impl_stat.cpp
    log.cpp
    probes.cpp
)

PEERDIR(
    cloud/filestore/config
    cloud/filestore/libs/client
    cloud/filestore/libs/fuse/protos
    cloud/filestore/libs/service

    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics

    library/cpp/lwtrace
)

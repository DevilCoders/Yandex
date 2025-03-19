LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    hive_proxy.cpp
)

PEERDIR(
    cloud/storage/core/libs/kikimr
    library/cpp/actors/core
    ydb/core/base
)

END()

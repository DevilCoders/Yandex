LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    actorsystem.cpp
    components.cpp
    events.cpp
    helpers.cpp
)

PEERDIR(
    cloud/storage/core/libs/common
    cloud/storage/core/libs/diagnostics
    library/cpp/actors/core
    library/cpp/actors/wilson
    library/cpp/lwtrace
    ydb/core/base
    ydb/core/protos
)

END()

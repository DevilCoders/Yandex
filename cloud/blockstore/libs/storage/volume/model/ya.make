LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    checkpoint.cpp
    client_state.cpp
    merge.cpp
    requests_inflight.cpp
    stripe.cpp
    volume_throttling_policy.cpp
)

PEERDIR(
    cloud/blockstore/libs/common
    cloud/blockstore/libs/storage/protos
    cloud/blockstore/libs/throttling

    library/cpp/actors/core
    library/cpp/containers/intrusive_rb_tree
)

END()

RECURSE_FOR_TESTS(ut)

UNITTEST_FOR(cloud/blockstore/libs/storage/volume/model)

OWNER(g:cloud-nbs)

SRCS(
    client_state_ut.cpp
    merge_ut.cpp
    requests_inflight_ut.cpp
    stripe_ut.cpp
    volume_throttling_policy_ut.cpp
)

END()

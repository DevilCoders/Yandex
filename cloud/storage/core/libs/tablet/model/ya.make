LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    commit.cpp
    partial_blob_id.cpp
)

PEERDIR(
    cloud/storage/core/libs/common
)

END()

RECURSE_FOR_TESTS(
    ut
)

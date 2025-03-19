LIBRARY()

OWNER(g:cloud-nbs)

SRCS(
    endpoints.cpp
    endpoints_test.cpp
    keyring.cpp
)

PEERDIR(
    cloud/storage/core/libs/common

    library/cpp/protobuf/util
)

END()

RECURSE_FOR_TESTS(ut)
RECURSE_FOR_TESTS(ut/bin)

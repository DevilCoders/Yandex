LIBRARY()

OWNER(
    mvel
    gotmanov
)

PEERDIR(
    contrib/libs/protobuf
)

SRCS(
    fuzz_policy.cpp
    fuzzy_output.cpp
    fuzzer.cpp
)

END()

UNITTEST()

OWNER(
    gotmanov
)

PEERDIR(
    kernel/proto_fuzz
    library/cpp/diff
)

SRCS(
    test.proto
    fuzzy_output_ut.cpp
    fuzzer_ut.cpp
)

END()

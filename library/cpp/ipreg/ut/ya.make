UNITTEST()

OWNER(
    g:geotargeting
)

SRCS(
    address_ut.cpp
    checker_ut.cpp
    format_ut.cpp
    merge_ut.cpp
    range_ut.cpp
    reader_ut.cpp
    split_ut.cpp
    test_helpers.cpp
    util_helpers_coarse_ut.cpp
    util_helpers_completeness_ut.cpp
    util_helpers_merge_ut.cpp
    util_helpers_monotonic_ut.cpp
    util_helpers_patch_ut.cpp
    util_helpers_stubs_ut.cpp
    writer_ut.cpp
)

PEERDIR(
    library/cpp/ipreg
)

DATA(
    arcadia/library/cpp/ipreg/ut/testIPREG.merge.a
    arcadia/library/cpp/ipreg/ut/testIPREG.merge.b
    arcadia/library/cpp/ipreg/ut/testIPREG.parse
    arcadia/library/cpp/ipreg/ut/testIPREG.split
)

END()

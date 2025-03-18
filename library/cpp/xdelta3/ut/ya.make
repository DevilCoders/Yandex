GTEST()

OWNER(g:bsyeti)

SIZE(SMALL)

SRCS(
    merge_patches_ut.cpp
    merge_proto_ut.cpp
)

PEERDIR(
    library/cpp/xdelta3/state
    library/cpp/xdelta3/ut/rand_data

    contrib/libs/xdelta3
)

END()

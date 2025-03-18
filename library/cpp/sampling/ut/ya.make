OWNER(yazevnul)

UNITTEST_FOR(library/cpp/sampling)

NO_WSHADOW()

SRCS(
    alias_method_ut.cpp
    roulette_wheel_ut.cpp
    sampling_tree_ut.cpp
)

PEERDIR(
    library/cpp/accurate_accumulate
)

END()

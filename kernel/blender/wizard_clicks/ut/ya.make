UNITTEST_FOR(kernel/blender/wizard_clicks)

OWNER(
    g:blender
)

SRCS(
    bigrams_ut.cpp
    factors_ut.cpp
)

PEERDIR(
    library/cpp/scheme
    library/cpp/scheme/ut_utils
)

END()

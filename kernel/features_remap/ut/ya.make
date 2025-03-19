OWNER(ilnurkh)

UNITTEST_FOR(kernel/features_remap)

SRCS(
    features_remap_ut.cpp
)

PEERDIR(
    kernel/web_l2_factors_info # just get some slice with pre-set features-infos
)

END()

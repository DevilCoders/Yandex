UNITTEST()

OWNER(
    leo
    kostik
)

PEERDIR(
    ADDINCL kernel/groupattrs
)

SRCDIR(kernel/groupattrs)

SRCS(
    attrweightprop_ut.cpp
    metainfo_ut.cpp
    config_ut.cpp
)

END()

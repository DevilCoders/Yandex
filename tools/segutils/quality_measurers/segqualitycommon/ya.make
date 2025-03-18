LIBRARY()

OWNER(velavokr)

SRCS(
    qualitycommon.cpp
)

PEERDIR(
    kernel/segmentator
    kernel/segnumerator
    kernel/segutils
    kernel/tarc/disk
    kernel/tarc/iface
    kernel/tarc/markup_zones
    library/cpp/getopt/small
    library/cpp/lcs
    library/cpp/numerator
    tools/segutils/segcommon
)

END()

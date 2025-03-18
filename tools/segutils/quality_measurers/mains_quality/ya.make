PROGRAM()

OWNER(velavokr)

SRCS(
    mains_quality.cpp
)

PEERDIR(
    kernel/segmentator
    kernel/tarc/disk
    tools/segutils/quality_measurers/segqualitycommon
    tools/segutils/segcommon
    yweb/news/tools/pr_meter_lib
)

END()

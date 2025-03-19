LIBRARY()

OWNER(ilnurkh)

PEERDIR(
    kernel/synnorm
    library/cpp/archive
)

SRCS(
    synnorm_standalone.cpp
)

FROM_SANDBOX(153323947 OUT_NOAUTO synnorm.gzt.bin)

ARCHIVE_ASM(
    NAME SynnormGztBinData
    synnorm.gzt.bin
    search/wizard/data/wizard/language/stopword.lst
)

END()

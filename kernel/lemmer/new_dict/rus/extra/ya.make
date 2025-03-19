LIBRARY()

OWNER(g:morphology)

SRCS(
    GLOBAL rus_extra.cpp
)

RUN_PROGRAM(
    dict/tools/make_morphdict/main make-all --language rus --bastard-freq 0 --input lister.txt
    STDOUT rus.extra.dict.bin
    IN lister.txt
    OUT_NOAUTO rus.extra.dict.bin
)

ARCHIVE_ASM(
    NAME RusExtraDict
    DONTCOMPRESS
    ${BINDIR}/rus.extra.dict.bin
)

PEERDIR(
    kernel/lemmer/core
    kernel/lemmer/new_dict/common
    kernel/lemmer/new_dict/rus/main_with_trans
)

END()

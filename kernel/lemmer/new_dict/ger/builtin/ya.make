LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py ger.dict.bin.gz out.dict.bin
    IN ger.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME GerDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL ger_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/ger/main
    kernel/lemmer/new_dict/builtin
)

END()

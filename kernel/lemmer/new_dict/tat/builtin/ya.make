LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py tat.dict.bin.gz out.dict.bin
    IN tat.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME TatDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL tat_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/tat/main
    kernel/lemmer/new_dict/builtin
)

END()

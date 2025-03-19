LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py ita.dict.bin.gz out.dict.bin
    IN ita.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME ItaDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL ita_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/builtin
    kernel/lemmer/new_dict/ita/main
)

END()

LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py por.dict.bin.gz out.dict.bin
    IN por.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME PorDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL por_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/por/main
    kernel/lemmer/new_dict/builtin
)

END()

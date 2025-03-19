LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py bel.dict.bin.gz out.dict.bin
    IN bel.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME BelDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL bel_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/bel/main
    kernel/lemmer/new_dict/builtin
)

END()

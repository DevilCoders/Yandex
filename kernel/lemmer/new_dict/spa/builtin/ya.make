LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py spa.dict.bin.gz out.dict.bin
    IN spa.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME SpaDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL spa_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/spa/main
    kernel/lemmer/new_dict/builtin
)

END()

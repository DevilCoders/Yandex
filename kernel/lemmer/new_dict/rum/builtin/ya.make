LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py rum.dict.bin.gz out.dict.bin
    IN rum.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME RumDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL rum_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/rum/main
    kernel/lemmer/new_dict/builtin
)

END()

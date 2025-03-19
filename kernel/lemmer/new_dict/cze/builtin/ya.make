LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py cze.dict.bin.gz out.dict.bin
    IN cze.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME CzeDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL cze_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/cze/main
    kernel/lemmer/new_dict/builtin
)

END()

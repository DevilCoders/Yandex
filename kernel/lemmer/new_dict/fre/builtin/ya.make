LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py fre.dict.bin.gz out.dict.bin
    IN fre.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME FreDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL fre_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/fre/main
    kernel/lemmer/new_dict/builtin
)

END()

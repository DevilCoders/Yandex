LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py ukr.dict.bin.gz out.dict.bin
    IN ukr.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME UkrDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL ukr_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/builtin
    kernel/lemmer/new_dict/ukr/main_with_trans
)

END()

LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py kaz.dict.bin.gz out.dict.bin
    IN kaz.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME KazDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL kaz_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/kaz/main_with_trans
    kernel/lemmer/new_dict/builtin
)

END()

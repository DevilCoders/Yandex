LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py bul.dict.bin.gz out.dict.bin
    IN bul.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME BulDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL bul_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/bul/main
    kernel/lemmer/new_dict/builtin
)

END()

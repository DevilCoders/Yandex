LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py ara.dict.bin.gz out.dict.bin
    IN ara.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME AraDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL ara_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/ara/main
    kernel/lemmer/new_dict/builtin
)

END()

LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py pol.dict.bin.gz out.dict.bin
    IN pol.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME PolDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL pol_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/pol/main
    kernel/lemmer/new_dict/builtin
)

END()

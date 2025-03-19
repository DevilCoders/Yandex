LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py eng.dict.bin.gz out.dict.bin
    IN eng.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME EngDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL eng_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/builtin
    kernel/lemmer/new_dict/eng/main
    kernel/lemmer/new_dict/eng/basic
)

END()

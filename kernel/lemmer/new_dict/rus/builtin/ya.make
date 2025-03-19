LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py rus.dict.bin.gz out.dict.bin
    IN rus.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME RusDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL rus_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/builtin
    kernel/lemmer/new_dict/rus/basic
    kernel/lemmer/new_dict/rus/main_with_trans
    kernel/lemmer/new_dict/rus/extra
)

END()

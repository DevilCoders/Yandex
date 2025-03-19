LIBRARY()

OWNER(
    gotmanov
)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py rus.dict.bin.famn.m_f.gz out.dict.bin
    IN rus.dict.bin.famn.m_f.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME RusFioDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL rus_fio_language.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/builtin
    kernel/lemmer/new_dict/common
)

END()

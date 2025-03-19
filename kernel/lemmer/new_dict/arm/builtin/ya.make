LIBRARY()

OWNER(g:morphology)

PYTHON(
    ${ARCADIA_ROOT}/kernel/lemmer/new_dict/builtin/unzip.py arm.dict.bin.gz out.dict.bin
    IN arm.dict.bin.gz
    OUT_NOAUTO out.dict.bin
)

ARCHIVE_ASM(
    NAME ArmDict
    DONTCOMPRESS
    ${BINDIR}/out.dict.bin
)

SRCS(
    GLOBAL arm_builtin.cpp
)

PEERDIR(
    kernel/lemmer/new_dict/arm/main
    kernel/lemmer/new_dict/builtin
)

END()

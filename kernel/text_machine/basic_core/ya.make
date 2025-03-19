LIBRARY()

OWNER(
    g:factordev
    edik
    gotmanov
)

SRCS(
    GLOBAL best_core.cpp
    GLOBAL full_core.cpp
    tm_common.cpp
)

PEERDIR(
    kernel/text_machine/parts
)

RUN_PROGRAM(kernel/text_machine/tm_codegen --namespace NTextMachine --namespace NBasicCore
    tm_common_gen.in
    tm_best_gen.in
    tm_full_gen.in
    --output-path ${ARCADIA_BUILD_ROOT}/kernel/text_machine/basic_core
    IN
    tm_common_gen.in
    tm_best_gen.in
    tm_full_gen.in
    OUT_NOAUTO
    tm_common_gen.cpp tm_common_gen.h
    tm_best_gen.cpp tm_best_gen.h
    tm_full_gen.cpp tm_full_gen.h
)

END()

LIBRARY()

OWNER(
    gotmanov
    agusakov
)

PEERDIR(
    kernel/text_machine/interface
)

SRCS(
    plain_text_aggregator.cpp
    plain_text_machine.cpp
)

RUN_PROGRAM(kernel/text_machine/tm_codegen --namespace NTextMachine --namespace NPlainText
    pt_aggregator_gen.in
    --output-path ${ARCADIA_BUILD_ROOT}/kernel/text_machine/plain_text
    IN
    pt_aggregator_gen.in
    OUT_NOAUTO
    pt_aggregator_gen.cpp pt_aggregator_gen.h
)

END()

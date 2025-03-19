LIBRARY()

OWNER(
    g:base
    g:factordev
    gotmanov
    edik
)

PEERDIR(
    kernel/text_machine/interface
    kernel/text_machine/module
    kernel/text_machine/parts/common
    library/cpp/json
)

SRCS(
    annotation_accumulator_parts.cpp
    chain_accumulator.cpp
    coordination_accumulator.cpp
    plane_accumulator_parts.cpp
    sequence_accumulator.cpp
    window_accumulator.cpp
)

GENERATE_ENUM_SERIALIZATION(annotation_accumulator_parts.h)

END()

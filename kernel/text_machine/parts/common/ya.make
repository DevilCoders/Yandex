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
    library/cpp/enumbitset
    library/cpp/vec4
)

SRCS(
    break_detector.cpp
    features_helper.cpp
    queries_helper.cpp
    seq4.cpp
    scatter.cpp
    storage.cpp
    structural_stream.cpp
    switch_hit_codegen.cpp
    types.cpp
)

GENERATE_ENUM_SERIALIZATION(storage.h)
GENERATE_ENUM_SERIALIZATION(types.h)

END()

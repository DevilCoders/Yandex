LIBRARY()

OWNER(
    g:base
    g:factordev
    gotmanov
)

PEERDIR(
    kernel/lingboost
    kernel/reqbundle
    kernel/text_machine/interface
    kernel/text_machine/proto
    library/cpp/offroad/codec
    library/cpp/offroad/custom
    library/cpp/offroad/flat
)

SRCS(
    expansions_resolver.cpp
    hits_map.cpp
    hits_serializer.cpp
)

RESOURCE(
    text_machine_hit_model_v1 text_machine_hit_model_v1
    text_machine_annotation_model_v1 text_machine_annotation_model_v1
)

END()

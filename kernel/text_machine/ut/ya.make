UNITTEST()

OWNER(
    edik
    gotmanov
)

PEERDIR(
    kernel/text_machine/basic_core
    kernel/text_machine/util
)

SRCS(
    aggregator_ut.cpp
    bag_tracker_ut.cpp
    feature_id_ut.cpp
    hits_serializer_ut.cpp
    precalculated_table_ut.cpp
    seq4_ut.cpp
    sequence_accumulator_ut.cpp
    structural_stream_ut.cpp
    weights_ut.cpp
    window_accumulator_ut.cpp
)

END()

UNITTEST()

OWNER(
    crossby
)

SRCS(
    transform_ut.cpp
    similarity_ut.cpp
    slicer_ut.cpp
    aggregator_ut.cpp
    calcer_ut.cpp
)

PEERDIR(
    kernel/vector_machine
    library/cpp/protobuf/util
)

BASE_CODEGEN(kernel/vector_machine/codegen sim_aggr)

END()

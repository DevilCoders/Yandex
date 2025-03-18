UNITTEST()

OWNER(tobo)

PEERDIR(
    library/cpp/vowpalwabbit
    library/cpp/resource
)

RESOURCE(
    vw-model-811.txt /library/cpp/vowpalwabbit/ut/vw-model-811
)

RESOURCE(
    vw-model-820.txt /library/cpp/vowpalwabbit/ut/vw-model-820
)

RESOURCE(
    vw-model-833.txt /library/cpp/vowpalwabbit/ut/vw-model-833
)

RESOURCE(
    vw-model-readable.txt /library/cpp/vowpalwabbit/ut/vw-model-readable
)

SRCS(
    vowpal_wabbit_model_ut.cpp
    vowpal_wabbit_predictor_ut.cpp
    readable_repr_ut.cpp
)

END()

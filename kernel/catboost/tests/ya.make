UNITTEST_FOR(kernel/catboost)

OWNER(
    g:matrixnet
)

PEERDIR(
    catboost/yandex_specific/libs/mn_sse_conversion
    kernel/catboost
    kernel/relevfml/testmodels
    library/cpp/archive
    quality/relev_tools/mx_model_lib
)

BUILD_CATBOOST(catboost_adult.cbm CatboostAdultModel)
BUILD_CATBOOST(catboost_float_model.cbm CatboostFloatModel)

SRCS(
    catboost_calcer_ut.cpp
)

END()

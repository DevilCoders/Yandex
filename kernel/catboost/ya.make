LIBRARY()

OWNER(vavinov)

PEERDIR(
    catboost/libs/model
    kernel/factor_slices
    kernel/factor_storage
    kernel/matrixnet
    library/cpp/threading/thread_local
)

SRCS(
    catboost_calcer.cpp
)

END()

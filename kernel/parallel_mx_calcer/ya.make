LIBRARY()

OWNER(
    g:middle
    g:base
    kulikov
    swarmer
)

SRCS(
    parallel_mx_calcer.cpp
)

PEERDIR(
    kernel/factor_storage
    kernel/bundle
    kernel/matrixnet
    kernel/searchlog
    library/cpp/containers/2d_array
    library/cpp/threading/async_task_batch
)

END()

LIBRARY()

OWNER(alex-sh)

SRCS(
    options.h
    pool.h
    splitter.h
    linear_regression.h
    prune.h
    least_squares_tree.h
    loss_functions.h
    loss_functions.cpp
    matrix.h
    quad.h
    pairwise.h
    logistic.h
    logistic_query_reg.h
    rank.h
    compositions.h
    cross_validation.h
    transform_features.h
    optimization.h
    surplus.h
    surplus_new.h
)

PEERDIR(
    kernel/ethos/lib/reg_tree_applier_lib
    kernel/matrixnet
    library/cpp/scheme
    library/cpp/getopt/small
    library/cpp/linear_regression
    library/cpp/statistics
)

END()

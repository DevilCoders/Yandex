UNITTEST_FOR(kernel/nn/tf_apply)

OWNER(boyalex)

PEERDIR(
    library/cpp/testing/unittest
    kernel/nn/tf_apply
    contrib/libs/tf/all_ops
)

SIZE(SMALL)

SRCS(
    input_test_ut.cpp
    util_test_ut.cpp
    applier_test_ut.cpp
)

DATA(
    sbr://616201911  # sum.pb: simple model with 1 input, dim=2, sums input features
    sbr://624350243  # sum_2_inputs.pb: simple model with 2 inputs, dim=1, sums inputs
    sbr://629245684  # x5.pb: x5 element-wise, dim = 2
    sbr://642115193  # sum_sparse.pb: simple model with 1 input, sum of sparse row, dim = 1000
    sbr://642119943  # sum_sparse_2_inputs.pb, sum of sum of 2 sparse inputs, dim = 1000
    sbr://710379623  # const_square.pb, calculate 10**2=100, no input
)

END()

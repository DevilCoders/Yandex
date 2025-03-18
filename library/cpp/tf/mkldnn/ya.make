LIBRARY()

OWNER(
    dbakshee
    g:danet-dev
)

LICENSE(Apache-2.0)

NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/deprecated/onednn
    contrib/libs/cxxsupp/openmp
    contrib/libs/tf
)

ADDINCL(contrib/deprecated/onednn/include)

SRCS(
    GLOBAL mkldnn_layout_pass.cc
    GLOBAL mkldnn_tfconversion_pass.cc
    GLOBAL mkldnn_add_op.cc
    GLOBAL mkldnn_concat_op.cc
    GLOBAL mkldnn_conv_ops.cc
    GLOBAL mkldnn_maxpooling_op.cc
    GLOBAL mkldnn_relu_op.cc
    GLOBAL mkldnn_tfconv_op.cc
    GLOBAL mkldnn_ops.cc
    GLOBAL mkldnn_undoweights_op.cc
)

END()

PROGRAM()

OWNER(g:matrixnet)

NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/libs/tf/tests/lib
    library/cpp/tf/mkldnn
)

SRCDIR(library/cpp/tf/mkldnn)

SRCS(
    mkldnn_layout_pass_test.cc
)

END()

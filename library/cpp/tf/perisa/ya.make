LIBRARY()

OWNER(g:matrixnet)

IF (MSVC)
    CFLAGS(-DCOMPILER_MSVC)
ENDIF()

PEERDIR(
    contrib/libs/tf
)

SRCS(
    GLOBAL custom_kernel_creator.cpp
)

SRC_C_AVX(avx/bias_op.cpp)

SRC_C_AVX(avx/cwise_op_mul_1.cpp)

SRC_C_AVX(avx/cwise_ops_common.cpp)

SRC_C_AVX(avx/matmul_op.cpp)

END()

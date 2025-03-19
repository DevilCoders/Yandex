LIBRARY()

OWNER(
    boyalex
)

NO_COMPILER_WARNINGS()

PEERDIR(
    contrib/libs/eigen
    contrib/libs/farmhash
    contrib/libs/highwayhash
    contrib/libs/nsync
    contrib/libs/protobuf
    contrib/libs/re2
    contrib/libs/snappy
    contrib/libs/tf/proto
    contrib/libs/zlib
)

IF (TENSORFLOW_WITH_XLA)
    PEERDIR(
        contrib/libs/tf/tensorflow/compiler
    )
ENDIF()

ADDINCL(
    contrib/libs/eigen
    contrib/libs/farmhash
    contrib/libs/giflib
    contrib/libs/highwayhash
    contrib/libs/libpng
    contrib/libs/sqlite3
    contrib/libs/libjpeg-turbo
    contrib/restricted/gemmlowp

    GLOBAL contrib/libs/tf
)

CFLAGS(
    -DEIGEN_AVOID_STL_ARRAY
    -DEIGEN_MPL2_ONLY
    -DGRPC_ARES=0
    -DPB_FIELD_16BIT=1
    -DSQLITE_OMIT_DEPRECATED
    -DSQLITE_OMIT_LOAD_EXTENSION
    -DTF_USE_SNAPPY
    -D_FORCE_INLINES
)

IF (TENSORFLOW_WITH_XLA)
    CFLAGS(
        -DTENSORFLOW_EAGER_USE_XLA
    )
ENDIF()

SRCS(
    GLOBAL contrib/libs/tf/tensorflow/core/common_runtime/direct_session.cc
    GLOBAL contrib/libs/tf/tensorflow/core/common_runtime/parallel_concat_optimizer.cc
    GLOBAL contrib/libs/tf/tensorflow/core/common_runtime/sycl/sycl_device_factory.cc
    GLOBAL contrib/libs/tf/tensorflow/core/common_runtime/threadpool_device.cc
    GLOBAL contrib/libs/tf/tensorflow/core/common_runtime/threadpool_device_factory.cc
    GLOBAL contrib/libs/tf/tensorflow/core/graph/mkl_layout_pass.cc
    GLOBAL contrib/libs/tf/tensorflow/core/graph/mkl_tfconversion_pass.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/as_string_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/base64_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/batch_kernels.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/batch_matmul_op_complex.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/batch_matmul_op_real.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/batch_norm_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/batchtospace_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/bcast_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/betainc_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/bias_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/bincount_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/bitcast_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/broadcast_to_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/bucketize_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/candidate_sampler_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cast_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/check_numerics_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/compare_and_bitpack_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/concat_lib_cpu.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/constant_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/control_flow_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_add_1.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_add_2.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_div.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_floor.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_floor_div.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_floor_mod.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_greater.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_greater_equal.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_maximum.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_minimum.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_mul_1.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_mul_2.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_neg.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_real.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_reciprocal.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_rsqrt.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_select.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_sigmoid.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_sqrt.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_square.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_squared_difference.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/cwise_op_sub.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/example_parsing_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/fact_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/fake_quant_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/fft_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/function_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/functional_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/fused_batch_norm_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/guarantee_const_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/identity_n_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/identity_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/immutable_constant_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/inplace_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/list_kernels.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/logging_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/matmul_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/no_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/random_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/reduction_ops_mean.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/reduction_ops_sum.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/relu_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/reshape_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/resource_variable_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/scoped_allocator_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/sendrecv_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/session_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/set_kernels.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/shape_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/unary_ops_composition.cc
    GLOBAL contrib/libs/tf/tensorflow/core/kernels/variable_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/array_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/batch_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/bitwise_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/candidate_sampling_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/control_flow_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/data_flow_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/function_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/functional_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/linalg_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/list_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/logging_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/math_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/nn_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/no_op.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/parsing_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/random_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/resource_variable_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/scoped_allocator_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/sendrecv_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/set_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/spectral_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/state_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/ops/string_ops.cc
    GLOBAL contrib/libs/tf/tensorflow/core/user_ops/fact.cc
    GLOBAL contrib/libs/tf/tensorflow/core/util/proto/local_descriptor_pool_registration.cc
    GLOBAL contrib/libs/tf/tensorflow/core/util/tensor_bundle/tensor_bundle.cc
)

END()

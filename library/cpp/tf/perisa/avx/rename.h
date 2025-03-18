#pragma once

#define ARCADIA_BUILD_ROOT_SKIP_KERNEL_REGISTRATION

// What ::Eigen symbols to redefine?
// - Redefine operations *Op (use nm to check)
// - Redefine BinaryOpShared
// - Do not redefine TensorOpCost

#define BiasGradOp BiasGradOpAvx
#define BiasOp BiasOpAvx
#define BinaryOp BinaryOpAvx
#define BinaryOpShared BinaryOpSharedAvx
#define MatMulOp MatMulOpAvx
#define TensorAssignOp TensorAssignOpAvx
#define TensorBroadcastingOp TensorBroadcastingOpAvx
#define TensorContractionOp TensorContractionOpAvx
#define TensorCwiseBinaryOp TensorCwiseBinaryOpAvx
#define TensorCwiseUnaryOp TensorCwiseUnaryOpAvx

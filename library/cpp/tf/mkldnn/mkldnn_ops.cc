/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.
   Copyright 2017-2018 YANDEX LLC

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#if defined(ARCADIA_BUILD_ROOT)

#include "tensorflow/core/framework/common_shape_fns.h"
#include "tensorflow/core/framework/op_kernel.h"
#include "tensorflow/core/util/tensor_format.h"

namespace tensorflow {
    REGISTER_OP("_MkldnnConcat")
        .Input("concat_dim: int32")
        .Input("values: N * T")
        .Input("mkl_concat_dim: uint8")
        .Input("mkl_values: N * uint8")
        .Output("output: T")
        .Output("mkl_output: uint8")
        .Attr("N: int >= 2")
        .Attr("T: type")
        .Attr(GetConvnetDataFormatAttrString())
        .SetShapeFn([](shape_inference::InferenceContext* c) {
          return shape_inference::ConcatShape(c, c->num_inputs() - 3);
        })
        .Doc(R"doc(
MKLDNN version of Concat operation.
NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnConcatV2")
        .Input("values: N * T")
        .Input("axis: Tidx")
        .Input("mkl_values: N * uint8")
        .Input("mkl_axis: uint8")
        .Output("output: T")
        .Output("mkl_output: uint8")
        .Attr("N: int >= 2")
        .Attr("T: type")
        .Attr("Tidx: {int32, int64} = DT_INT32")
        .Attr(GetConvnetDataFormatAttrString())
        .SetShapeFn(shape_inference::ConcatV2Shape)
        .Doc(R"doc(
MKLDNN version of ConcatV2 operation.
NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnConv2D")
        .Input("input: T")
        .Input("filter: T")
        .Input("mkl_input: uint8")
        .Input("mkl_filter: uint8")
        .Output("output: T")
        .Output("mkl_output: uint8")
        .Attr("T: {half, float, double}")
        .Attr("strides: list(int)")
        .Attr("use_cudnn_on_gpu: bool = true")
        .Attr("groups: int = 1")
        .Attr("leak: float = 1")            /* leak=1 means no relu */
        .Attr("use_weights_as_given: bool") /* no default value */
        .Attr("openmp_threads: int = 0")
        .Attr(GetPaddingAttrString())
        .Attr(GetConvnetDataFormatAttrString())
        .SetShapeFn(shape_inference::Conv2DShape)
        .Doc(R"doc(
MKLDNN version of Conv2D operator.

NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnConv2DWithBias")
        .Input("input: T")
        .Input("filter: T")
        .Input("bias: T")
        .Input("mkl_input: uint8")
        .Input("mkl_filter: uint8")
        .Input("mkl_bias: uint8")
        .Output("output: T")
        .Output("mkl_output: uint8")
        .Attr("T: {half, float, double}")
        .Attr("strides: list(int)")
        .Attr("use_cudnn_on_gpu: bool = true")
        .Attr("groups: int = 1")
        .Attr("leak: float = 1")            /* leak=1 means no relu */
        .Attr("use_weights_as_given: bool") /* no default value */
        .Attr("openmp_threads: int = 0")
        .Attr(GetPaddingAttrString())
        .Attr(GetConvnetDataFormatAttrString())
        .Doc(R"doc(
MKLDNN version of Conv2D + BiasAdd operator.

NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnMaxPool")
        .Attr("T: {float, half} = DT_FLOAT")
        .Attr("ksize: list(int) >= 4")
        .Attr("strides: list(int) >= 4")
        .Attr(GetPaddingAttrString())
        .Attr(GetConvnetDataFormatAttrString())
        .Attr("workspace_enabled: bool = false")
        .Input("input: T")
        .Input("mkl_input: uint8")
        .Output("output: T")
        .Output("mkl_output: uint8")
        .SetShapeFn(shape_inference::MaxPoolShape)
        .Doc(R"doc(
MKLDNN version of MaxPool operator.

NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnToTf")
        .Input("input: T")
        .Input("mkl_input: uint8")
        .Output("output: T")
        .Attr("T: {half, float, double}")
        .Attr(GetConvnetDataFormatAttrString())
        .Doc(R"doc(
MKLDNN operator to convert a tensor from MKLDNN layout to TensorFlow layout.

NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnAdd")
        .Input("x: T")
        .Input("y: T")
        .Input("mkl_x: uint8")
        .Input("mkl_y: uint8")
        .Output("z: T")
        .Output("mkl_z: uint8")
        .Attr("T: {float}")
        .Attr(GetConvnetDataFormatAttrString())
        .Doc(R"doc(
MKLDNN Add operation.

NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_MkldnnRelu")
        .Input("x: T")
        .Input("mkl_x: uint8")
        .Output("z: T")
        .Output("mkl_z: uint8")
        .Attr("T: {float}")
        .Attr("leak: float = 0")
        .Attr(GetConvnetDataFormatAttrString())
        .Doc(R"doc(
MKLDNN Relu operation.

NOTE Do not invoke this operator directly in Python. Graph rewrite pass is
expected to invoke these operators.
)doc");

    REGISTER_OP("_UndoWeightsMkldnn")
        .Input("input: T")
        .Output("output: T")
        .Attr("T: {float}")
        .Doc(R"doc(
Undo `MkldnnUseWeightsAsGiven` mode for hosts/cases where MKLDNN is not enabled.
Convert weights pretended to be a 'HWIO' tensor but actually contained as 'OIHW8' or such,
into actual 'HWIO' format for use by TF Conv2D operation.
)doc");
}

#endif /* ARCADIA_BUILD_ROOT */

TF tweaked for Yandex
=====================

What the tweaks?
----------------

The tweaks are marked by word `ARCADIA_BUILD_ROOT`.

When the tweaks are controlled by environment variables,
the variable is considered _set_ if its value can be interpreted
as a non-zero number or is a word starting with one of letters `tTyY`.

### DANET-style padding

By default, TensorFlow's default is "when the padding on both sides
(top vs bottom, right vs left) are off by one, then
_the bottom and right sides_ always get the one additional padded pixel."
When environment variable `DANET_STYLE_PADDING` is set, the
one extra pixel goes to _the top and left sides_.
This changes interpretation of `padding="SAME"` for odd total padding.

### `TF_ENABLE_MKLDNN`

This optimization improves performance of `Conv2D` and some other
TF operations on AVX machines. MKLDNN generates JIT code for these
operations. On non-AVX machines setting this environment variable
has no effect.

MKLDNN improves performance for graphs using `NCHW` data format.
Performance of graphs using TensorFlow native data format `NHWC`
will likely be degraded by MKLDNN.

The integration follows integration of MKL in TF-1.2, but
is expected not to interfere with that. MKL library is not used
by the MKLDNN integration.

### MKLDNN can use weights as is

This optimization improves performance of `Conv*` and allows
loading of the model into shared memory. The optimization
can be turned on by setting respective flag in `SessionOptions`.

In this mode, the filters for MKLDNN convolutions are assumed to
have necessary memory format and not reordered.

For example of loading the model into shared memory see
[tf-kartinki](https://a.yandex-team.ru/arc/trunk/arcadia/cv/imgclassifiers/tf_applicator/example/tf-kartinki.cpp)

### `TF_NUM_THREADS`

Default number of threads for TensorFlow can be set by this environment variable.
It is useful for debugging an application that does not care to limit the
thread pool.

### `TF_OPENCV_RESIZE`

Use Opencv for some `tf.image.resize_*` operations.
This makes image resize operations numerically compatible with Danet.


## Optimization passes on TF graph are added

* MKLDNN layout pass - locate `Conv2D` and other operations and
  convert them into `_Mkldnn*` operations
  (possibly with context, such as `Conv2D` + `BiasAdd` + `Relu`).
  The data in input/output tensors for the Mkldnn operations has special
  layout, described by `mkldnn_memory_desc_t`. The i/o tensors of each
  Mkldnn node are complemented with _metadata_ tensors conveying the format
  of the data tensors.

* TF conversion pass - insert format conversion operations
  on the edges coming out of Mkldnn nodes and not entering other Mkldnn nodes.

* MkldnnWeights pass - insert format conversion operations
  on the edges supplying weights to `_MkldnnConv2D*` operations.

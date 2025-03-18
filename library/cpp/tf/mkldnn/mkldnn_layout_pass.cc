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

#include <algorithm>
#include <functional>
#include <memory>
#include <queue>
#include <set>
#include <string>
#include <unordered_set>
#include <util/string/printf.h>
#include <utility>
#include <vector>
#include "tensorflow/core/common_runtime/function.h"
#include "tensorflow/core/common_runtime/optimization_registry.h"
#include "tensorflow/core/framework/node_def_util.h"
#include "tensorflow/core/graph/algorithm.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/node_builder.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/gtl/array_slice.h"
#include "tensorflow/core/lib/gtl/map_util.h"
#include "tensorflow/core/lib/hash/hash.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/public/session_options.h"
#include "tensorflow/core/util/tensor_format.h"

#include "mkldnn_layout_pass.h"
#include "mkldnn_util.h"
#include <util/system/cpu_id.h>

// Original (base) design, implementation, and description thereof can be found
// in file mkl_layout_pass.cc. This file preserves much of the original approach
// and style.
//
// Given a graph, MkldnnLayoutRewritePass() rewrites some nodes or merges
// subgraphs into _Mkldnn nodes. It makes several passes until no rewrite are
// possible.
//
// Graph and nodes
// ---------------
//
// Any node has four types of incident edges:
// - input edges (entering ordered list of input slots, one edge per slot, and
//   delivering some data or tensors)
// - output edges (leaving ordered list of output slots, any number of edges per
//   slot, and propagating some tensors along them)
// -- input and output control edges (not associated with a slot, not delivering
//   tensors, any number of edges, used by control flow operations and 'frames'
//   whatever it is)
//
// Nodes also have associated attributes, defined by node type. We cannot attach
// an arbitrary attribute at runtime.
//
// Example 1: Node `Conv2D` has 2 inputs: data on input slot 0, and filter on
// input slot 1, and one result on output slot 0. Format of the data is defined
// by the node attribute `data_format` (either `NHWC` or `NCHW`).
//
// Example 2: Node `Split` has split axis number on input  slot 0, and data on
// input slot 1, and some results on output slots 0, 1, ...  (number of valid
// output slots defined by  `num_split` attribute of the Split node).
//
// New _Mkldnn nodes produce data in a 'mkldnn format', which cannot be directly
// consumed by a non-_Mkldnn node, so another pass may add conversion nodes
// (named _MkldnnToTf, see mkldnn_tfconversiont_pass.cc). However, if an output
// of an _Mkldnn node goes to another _Mkldnn node, then no _MkldnnToTf
// conversion is necessary, because _Mkldnn nodes can consume any data format.
//
// To see what type of data it is fed, _Mkldnn node has a METADATA input slot
// for every normal input slot. The slots are numbered contiguously, e.g.
// _MkldnnConv2DWithBias node has input slots 0:data, 1:filter, 2:bias,
// 3:meta_data, 4:meta_filter, 5:meta_bias. And of course every _Mkldnn node
// (except _MkldnnToTf) has a METADATA output slot. (In the code we write
// 'mkl_xxx' instead of 'meta_xxx' for historical reasons, it should be fixed.)
//
// We do not support interleaved placement of METADATA slots (e.g. 0:data,
// 1:meta_data, 2:filter, 3:meta_filter, 4:bias, 5:meta_bias).
//
// If an _Mkldnn node B consumes data from an _Mkldnn node A (say, edge
// A:i-->B:j), then it also consumes the METADATA (that is edge A:na/2+i -->
// B:nb/2+j, where na and nb are number of output slots of node A and input
// slots of node B, respectively).
//
// If an _Mkldnn node B consumes data from a non-_Mkldnn node A, the the
// respective METADATA input slot of B is connected to a Dummy Metadata Tensor
// (DMT) operation. The DMT makes node B 'know' that the data is normal.
//
// We can remove a Node from the graph by `RemoveNode()`, and we can add edges
// to graph using `AddEdge()` or `AddControlEdge()`.
//
// We refrain from using `RemoveEdge()`, because removal of a node also removes
// its incident edges.
//
// To add a node to the graph, we use NodeBuilder.
//
// MergeNode
// ---------
//
// The function is called when a pair of nodes (succ, pred) match a pattern
// of an `minfo` structure. Rewriting proceeds then with these approximate
// steps:
//
// 1. Recognize the pattern of nodes to rewrite
//
// 2. Check that attributes of the nodes are valid and consistent, possibly
// record them
//
// 3. Check that rewriting would be semantically correct (e.g. pair A-->B cannot
// be rewritten to C if the output of A is also consumed by a node other than
// B).
//
// 4. Remember the nodes that must or should be removed after rewrite (e.g. when
// rewriting Max(X, Mul(X, leak)) into _MkldnnRelu(X, attr:leak) the Max and Mul
// nodes must be removed, but `leak` node should also be removed if Mul is the
// only node it feeds.
//
// 5. Build the new node using NodeBuilder. Set the Inputs and attribuites. Up
// to and including this step we may return a NOGO status which indicates that
// rewriting is not possible and should be skipped. Use TF_CPP_MIN_VLOG_LEVEL to
// identify such cases.
//
// 6. Commit the node into graph using NodeBuilder::Finalize. After this point
// we should not return a NOGO status.
//
// 7. Once the new node is commited into the graph, connect its output and
// control edges.
//
// 8. Finally, remove the nodes we have rewritten, and possibly other dangling
// nodes.

constexpr char* NOGO_STRING = "%i: NOGO. Will skip this transformation of graph.";
#define NOGO Status(error::Code::INVALID_ARGUMENT, Sprintf(NOGO_STRING, __LINE__))

namespace tensorflow {
    template <typename T>
    using TempArray = gtl::InlinedVector<T, 6>; // 6 is maximum number of node inputs

    static bool IsMkldnnOp(const Node* node, DataType T) {
        return mkl_op_registry::IsMkldnnOp(node->type_string(), T);
    }

    static bool IsConstOrImmutableConst(const Node* node) {
        return node->type_string() == "Const" || node->type_string() == "ImmutableConst";
    }

    static size_t kNodeMergeContextMaxDepth = 10;

    // Functions specific to operators to copy attributes
    // We need operator-specific function to copy attributes because the framework
    // does not provide any generic function for it.
    // NOTE: names are alphabetically sorted.
    template <typename T>
    void CopyOrSetAttr(const string& attr, NodeBuilder& nb, const Node* orig_node, const T& default_val) {
        T val;
        if (GetNodeAttr(orig_node->def(), attr, &val).ok()) {
            nb.Attr(attr, val);
        } else {
            nb.Attr(attr, default_val);
        }
    }
    static string GuessDataFormat(const Node* node);
    static void CopyAttrsAdd(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsBiasAddGrad(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsConcat(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsConcatV2(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsConv2D(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsFusedBatchNorm(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsIdentity(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsLRN(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsPooling(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsRelu(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsReshape(const Node* orig_node, NodeBuilder* nb);
    static void CopyAttrsSplit(const Node* orig_node, NodeBuilder* nb);

    class MkldnnLayoutRewritePass: public GraphOptimizationPass {
    public:
        MkldnnLayoutRewritePass() {
            // NOTE: names are alphabetically sorted.
            rinfo_.push_back({csinfo_.add,
                              GetMkldnnOpName(csinfo_.add),
                              CopyAttrsAdd, AddRewrite, nullptr});
            rinfo_.push_back({csinfo_.avg_pool,
                              GetMkldnnOpName(csinfo_.avg_pool),
                              CopyAttrsPooling, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.avg_pool_grad,
                              GetMkldnnOpName(csinfo_.avg_pool_grad),
                              CopyAttrsPooling, AlwaysRewrite, nullptr});
            // BiasAddGrad gets written into Conv2DWithBiasBackpropBias depending
            // on if context contains Conv2D.
            rinfo_.push_back({csinfo_.bias_add_grad,
                              csinfo_.mkl_conv2d_with_bias_backprop_bias,
                              CopyAttrsBiasAddGrad, ContextMatchRewrite,
                              &biasaddgrad_conv2dwithbias_context_});
            // BiasAddGrad gets written into BiasAddGrad depending on if context
            // contains MatMul.
            rinfo_.push_back({csinfo_.bias_add_grad, csinfo_.matmul,
                              CopyAttrsBiasAddGrad, ContextMatchRewrite,
                              &biasaddgrad_matmul_context_});
            rinfo_.push_back({csinfo_.concat,
                              GetMkldnnOpName(csinfo_.concat),
                              CopyAttrsConcat, RewriteIfNchwOrNhwc, nullptr});
            rinfo_.push_back({csinfo_.concatv2,
                              GetMkldnnOpName(csinfo_.concatv2),
                              CopyAttrsConcatV2, RewriteIfNchwOrNhwc, nullptr});
            rinfo_.push_back({csinfo_.conv2d,
                              GetMkldnnOpName(csinfo_.conv2d),
                              CopyAttrsConv2D, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.conv2d_grad_filter,
                              GetMkldnnOpName(csinfo_.conv2d_grad_filter),
                              CopyAttrsConv2D, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.conv2d_grad_input,
                              GetMkldnnOpName(csinfo_.conv2d_grad_input),
                              CopyAttrsConv2D, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.fused_batch_norm,
                              GetMkldnnOpName(csinfo_.fused_batch_norm),
                              CopyAttrsFusedBatchNorm, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.fused_batch_norm_grad,
                              GetMkldnnOpName(csinfo_.fused_batch_norm_grad),
                              CopyAttrsFusedBatchNorm, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.identity,
                              GetMkldnnOpName(csinfo_.identity),
                              CopyAttrsIdentity, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.lrn,
                              GetMkldnnOpName(csinfo_.lrn),
                              CopyAttrsLRN, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.lrn_grad,
                              GetMkldnnOpName(csinfo_.lrn_grad),
                              CopyAttrsLRN, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.max_pool,
                              GetMkldnnOpName(csinfo_.max_pool),
                              CopyAttrsPooling, NonDepthBatchWisePoolRewrite, nullptr});
            rinfo_.push_back({csinfo_.max_pool_grad,
                              GetMkldnnOpName(csinfo_.max_pool_grad),
                              CopyAttrsPooling, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.relu, csinfo_.mkl_relu,
                              CopyAttrsRelu, ReluRewrite, nullptr});
            rinfo_.push_back({csinfo_.relu_grad,
                              GetMkldnnOpName(csinfo_.relu_grad),
                              CopyAttrsRelu, AlwaysRewrite, nullptr});
            rinfo_.push_back({csinfo_.reshape,
                              GetMkldnnOpName(csinfo_.reshape),
                              CopyAttrsReshape, AlwaysRewrite, nullptr});

            // Add info about which ops to add workspace edge to and the slots.
            wsinfo_.push_back({csinfo_.lrn, csinfo_.lrn_grad, 0, 2, 1, 3});
            wsinfo_.push_back({csinfo_.max_pool, csinfo_.max_pool_grad, 0, 1, 1, 3});

            // Add a rule for merging nodes
            minfo_.push_back({csinfo_.mkl_conv2d, csinfo_.bias_add, 0,
                              csinfo_.mkl_conv2d_with_bias});

            minfo_.push_back({csinfo_.mkl_conv2d_with_bias, csinfo_.relu, 0,
                              csinfo_.mkl_conv2d_with_bias});

            minfo_.push_back({csinfo_.mkl_conv2d_with_bias, csinfo_.mkl_relu, 0,
                              csinfo_.mkl_conv2d_with_bias});

            minfo_.push_back({csinfo_.mul, csinfo_.maximum, 0,
                              csinfo_.mkl_relu});

            minfo_.push_back({csinfo_.mul, csinfo_.maximum, 1,
                              csinfo_.mkl_relu});

            minfo_.push_back({csinfo_.abs, csinfo_.mul, 0,
                              csinfo_.mkl_relu});

            minfo_.push_back({csinfo_.abs, csinfo_.mul, 1,
                              csinfo_.mkl_relu});

            minfo_.push_back({csinfo_.split, csinfo_.mkl_conv2d_with_bias, 0,
                              csinfo_.mkl_conv2d_with_bias});

            // We use maxhop of 10 based on empirical observations. Also, these are
            // maxhops in backward data-flow graph. Since input of forward nodes
            // (Conv2D) directly goes to backward nodes, we do not expect the
            // hop-distance would be more than few nodes.
            biasaddgrad_matmul_context_ = {csinfo_.bias_add_grad, csinfo_.matmul,
                                           kNodeMergeContextMaxDepth};

            biasaddgrad_conv2dwithbias_context_ = {csinfo_.bias_add_grad,
                                                   csinfo_.mkl_conv2d_with_bias,
                                                   kNodeMergeContextMaxDepth};

            cinfo_.push_back(&biasaddgrad_matmul_context_);
            cinfo_.push_back(&biasaddgrad_conv2dwithbias_context_);
        }

        // Standard interface to run pass
        Status Run(const GraphOptimizationPassOptions& options) override;

        // Helper function which does most of heavy lifting for rewriting
        // Mkldnn nodes to propagate Mkldnn tensor as additional output
        //
        // Extracts common functionality between Run public interface and
        // test interface.
        //
        // @return true, if and only if graph is mutated; false otherwise.
        bool RunPass(std::unique_ptr<Graph>* g);

        bool RunPassUndoWeights(std::unique_ptr<Graph>* g);

        /// Structure to specify the context information used in a node rewrite rule
        typedef struct {
            string node;    // Name of the node to be rewritten
            string fwd;     // Name of the node in the forward pass that this node
                            // corresponds to
            size_t max_hop; // Maximum number of hops the fwd is located
                            // from this node. If the fwd is farther than max_hop
                            // then we do not rewrite the node.
        } ContextInfo;

        /// Structure to specify the name of an original node, its new name after
        /// rewrite, the number of inputs to the original node, the function to
        /// be used to copy attributes for the op, and the rule (if any) which
        /// must hold for rewriting the node
        typedef struct {
            string name;     // Original name of op of the node in the graph
            string new_name; // New name of the op of the node in the graph
            // A function handler to copy attributes from an old node to a new node.
            std::function<void(const Node*, NodeBuilder*)> copy_attrs;
            // A rule under which to rewrite this node
            std::function<bool(const Node*, const ContextInfo* c)> rewrite_rule;
            // ContextInfo, if any, to be used for rewrite
            ContextInfo* context;
        } RewriteInfo;

        /// Structure to specify a forward op, a backward op, and the slot numbers
        /// in the forward and backward ops where we will add a workspace edge.
        typedef struct {
            string fwd_op;   // Name of a forward op in the graph
            string bwd_op;   // Name of a backward op in the graph
            int fwd_slot;    // Output slot in the forward op node where actual
                             // output tensor resides
            int bwd_slot;    // Input slot in the backward op node where actual
                             // input tensor resides
            int ws_fwd_slot; // Output slot in the forward op node where workspace
                             // edge is added
            int ws_bwd_slot; // Input slot in the backward op node where workspace
                             // edge is added
        } WorkSpaceInfo;

        /// Structure to specify information used in node merge
        typedef struct {
            string pred;     // Predecessor node string
            string succ;     // Successor node string
            int op;          // The operand no the predecessor node corresponds
                             // to the successor node
            string new_node; // Name of the node after merge
        } MergeInfo;

        /// Structure to store all constant strings
        /// NOTE: names are alphabetically sorted.
        struct {
            const string
                abs = "Abs",
                add = "Add",
                avg_pool = "AvgPool",
                avg_pool_grad = "AvgPoolGrad",
                bias_add = "BiasAdd",
                bias_add_grad = "BiasAddGrad",
                concat = "Concat",
                concatv2 = "ConcatV2",
                constant = "Const",
                conv2d = "Conv2D",
                conv2d_grad_input = "Conv2DBackpropInput",
                conv2d_grad_filter = "Conv2DBackpropFilter",
                fused_batch_norm = "FusedBatchNorm",
                fused_batch_norm_grad = "FusedBatchNormGrad",
                identity = "Identity",
                immutable_const = "ImmutableConst",
                lrn = "LRN",
                lrn_grad = "LRNGrad",
                matmul = "MatMul",
                maximum = "Maximum",
                max_pool = "MaxPool",
                max_pool_grad = "MaxPoolGrad",
                mkl_conv2d = "_MkldnnConv2D",
                mkl_conv2d_with_bias = "_MkldnnConv2DWithBias",
                mkl_conv2d_with_bias_backprop_bias = "_MkldnnConv2DWithBiasBackpropBias",
                mkl_relu = "_MkldnnRelu",
                mul = "Mul",
                relu = "Relu",
                relu_grad = "ReluGrad",
                reshape = "Reshape",
                split = "Split",
                undo_weights = "_UndoWeightsMkldnn",
                unpack = "Unpack";
        } csinfo_;

    private:
        /// Maintain info about nodes to rewrite
        std::vector<RewriteInfo> rinfo_;

        /// Maintain info about nodes to add workspace edge
        std::vector<WorkSpaceInfo> wsinfo_;

        /// Maintain info about nodes to be merged
        std::vector<MergeInfo> minfo_;

        /// Maintain info about nodes to rewrite
        static std::vector<ContextInfo*> cinfo_;

        /// Context variables used in referencing rules
        static ContextInfo biasaddgrad_matmul_context_;
        static ContextInfo biasaddgrad_conv2dwithbias_context_;

    private:
        inline bool IsConstOrImmutableConst(const Node* n) const {
            return n->type_string() == csinfo_.constant ||
                   n->type_string() == csinfo_.immutable_const;
        }

        inline bool IsProducingMkldnnTensor(const Node* n) const {
            return n->type_string().StartsWith("_Mkldnn") &&
                   n->type_string() != "_MkldnnToTf";
        }

        // Is OpDef::ArgDef a list type? It could be N * T or list(type).
        // Refer to opdef.proto for details of list type.
        inline bool ArgIsList(const OpDef::ArgDef& arg) const {
            return !arg.type_list_attr().empty() || !arg.number_attr().empty();
        }

        // Get length of a list in 'n' if 'arg' is of list type. Refer to
        // description of ArgIsList for definition of list type.
        inline int GetTensorListLength(const OpDef::ArgDef& arg, Node* n) {
            CHECK_EQ(ArgIsList(arg), true);
            int N = 0;
            const string attr_name = !arg.type_list_attr().empty()
                                         ? arg.type_list_attr()
                                         : arg.number_attr();
            if (!arg.type_list_attr().empty()) {
                std::vector<DataType> value;
                TF_CHECK_OK(GetNodeAttr(n->def(), attr_name, &value));
                N = value.size();
            } else {
                TF_CHECK_OK(GetNodeAttr(n->def(), attr_name, &N));
            }
            return N;
        }

        // Get the name of Mkldnn op from original TensorFlow op
        // We prefix 'Mkldnn' to the original op to get Mkldnn op.
        // TODO: We should move this to mkldnn_util.h.
        inline string GetMkldnnOpName(const string& name) const {
            // Prefix that we add to Tensorflow op name to construct Mkldnn op name.
            const char* const kMkldnnOpPrefix = "_Mkldnn";
            return string(kMkldnnOpPrefix) + name;
        }

        // Can op represented by node 'n' run on DEVICE_CPU?
        // Op can run on CPU with MKL if the runtime assigned device or the
        // user requested device contains device CPU, or both are empty.
        bool CanOpRunOnCPUDevice(const Node* n) {
            bool result = true;
            string reason;

            auto refersToNonCpu = [](const StringPiece& s) {
                return !s.empty() && !(str_util::StrContains(s, "cpu:") || str_util::StrContains(s, "CPU:"));
            };

            // If Op has been specifically assigned to a non-CPU device, then No.
            if (refersToNonCpu(n->assigned_device_name())) {
                result = false;
                reason = "Op has been assigned a runtime device that is not CPU: " + n->assigned_device_name();
            }

            // If user has specifically assigned this op to a non-CPU device, then No.
            if (refersToNonCpu(n->def().device())) {
                result = false;
                reason = "User has assigned a device that is not CPU: " + n->def().device();
            }

            if (result == false) {
                VLOG(1) << "MkldnnLayoutRewritePass: Skipping rewriting of the node "
                        << n->type_string() << ", reason: " << reason;
            }

            // Otherwise Yes.
            return result;
        }

        // Return a node that can be merged with input node 'n'
        //
        // @return pointer to the node if we can find such a
        // node. Otherwise, it returns nullptr.
        Node* CheckForNodeMerge(const Node* n) const;

        // Merge predecessor node with its successor.
        // Currently, we merge Conv2D with BiasAdd only.
        //
        // Input nodes succ and pred may be deleted if the call to
        // this function is successful. Attempt to use the pointers
        // after the call to function may result in undefined behaviors.
        //
        // @input g - input graph, succ - successor node, pred - predecessor node
        // @return Status::OK(), if merging is successful and supported.
        //         Returns appropriate Status error code otherwise.
        //         Graph is updated in case nodes are merged. Otherwise, it is
        //         not updated.
        Status MergeNode(std::unique_ptr<Graph>* g, Node* succ, Node* pred);
        Status MergeNode_MkldnnConv2DWithBias(std::unique_ptr<Graph>* g, Node* succ, Node* pred);
        Status MergeNode_MkldnnConv2DWithBias_relu(std::unique_ptr<Graph>* g, Node* succ, Node* pred);
        Status MergeNode_MkldnnConv2DWithBias_groups(std::unique_ptr<Graph>* g, Node* succ, Node* pred);
        Status MergeNode_MkldnnRelu(std::unique_ptr<Graph>* g, Node* succ, Node* pred);
        Status MergeNode_MkldnnRelu2(std::unique_ptr<Graph>* g, Node* succ, Node* pred);

        Status InsertUndoWeights(std::unique_ptr<Graph>* g, Node* conv);

        // Check if the node 'n' has any applicable rewrite rule
        // We check for 2 scenarios for rewrite.
        //
        // @return RewriteInfo* for the applicable rewrite rule
        const RewriteInfo* CheckForNodeRewrite(const Node* n) const;

        // Default rewrite rule to be used in scenario 1 for rewrite.
        // @return - true (since we want to always rewrite)
        static bool AlwaysRewrite(const Node* n, const ContextInfo* c = nullptr) {
            return true;
        }

        static bool RewriteIfNchwOrNhwc(const Node* n, const ContextInfo* c = nullptr) {
            auto guessed_data_format = GuessDataFormat(n);
            if (guessed_data_format != "NCHW" && guessed_data_format != "NHWC") {
                return false;
            }

            // Rewrite concat op only if its inputs were also rewrited.
            // Prevents bug with unsupported mkldnn-concat after dense layers in heads
            DataType T;
            TF_CHECK_OK(GetNodeAttr(n->def(), "T", &T));
            for (const Edge* e : n->in_edges()) {
                if (e->IsControlEdge())
                    continue;
                if (e->dst_input() == GetTensorDataIndex(0, n->num_inputs())) {
                    if (!IsMkldnnOp(e->src(), T))
                        return false;
                }
            }
            return true;
        }

        // Check if Add node can be rewritten as _MkldnnAdd
        static bool AddRewrite(const Node* n, const ContextInfo* c = nullptr) {
            DataType T;
            TF_CHECK_OK(GetNodeAttr(n->def(), "T", &T));
            if (!(T == DT_FLOAT))
                return false;
            for (const Edge* e : n->in_edges()) {
                if (e->IsControlEdge())
                    continue;
                if (e->dst_input() == GetTensorDataIndex(0, n->num_inputs())) {
                    if (!mkl_op_registry::IsMkldnnOp(e->src()->type_string(), T))
                        return false;
                }
                if (e->dst_input() == GetTensorDataIndex(1, n->num_inputs())) {
                    if (!mkl_op_registry::IsMkldnnOp(e->src()->type_string(), T))
                        return false;
                }
            }
            return true;
        }

        // Check if rewriting Relu into _MkldnnRelu can be done and is worth doing.
        static bool ReluRewrite(const Node* n, const ContextInfo* c = nullptr) {
            DataType T;
            TF_CHECK_OK(GetNodeAttr(n->def(), "T", &T));
            if (!(T == DT_FLOAT))
                return false;
            for (const Edge* e : n->in_edges()) {
                if (e->IsControlEdge())
                    continue;
                if (e->dst_input() == GetTensorDataIndex(0, n->num_inputs())) {
                    if (!IsMkldnnOp(e->src(), T))
                        return false;
                }
            }
            return true;
        }

        // Check if we are performing pooling on depth or batch. If it is, then we
        // do not rewrite MaxPool node to Mkldnn version.
        // @return - true (if it is not a depth/batch wise pooling case);
        //           false otherwise.
        static bool NonDepthBatchWisePoolRewrite(const Node* n, const ContextInfo* c) {
            CHECK_NOTNULL(n);

            string data_format_str;
            TensorFormat data_format;
            std::vector<int32> ksize, strides;
            CHECK_EQ(GetNodeAttr(n->def(), "ksize", &ksize).ok(), true);
            CHECK_EQ(GetNodeAttr(n->def(), "strides", &strides).ok(), true);
            CHECK_EQ(GetNodeAttr(n->def(), "data_format", &data_format_str).ok(),
                     true);
            CHECK_EQ(FormatFromString(data_format_str, &data_format), true);

            // Condition that specifies non-batch-wise and non-depth-wise pooling.
            if (GetTensorDim(ksize, data_format, 'N') == 1 &&
                GetTensorDim(strides, data_format, 'N') == 1 &&
                GetTensorDim(ksize, data_format, 'C') == 1 &&
                GetTensorDim(strides, data_format, 'C') == 1) {
                return true;
            }

            return false;
        }

        // Rewrite rule that uses context-information for matching,
        // used in scenario 2.
        //
        // @input - Node 'n' for which to search for matching context
        // @input - The context 'c' under which to rewrite
        // @return - true if we can rewrite node under context 'c';
        //           false otherwise.
        static bool ContextMatchRewrite(const Node* n, const ContextInfo* c);

        // Helper function that searches the matching contextinfo for the node.
        // Implements depth-first search in the data dependence graph for the
        // gradient op in the backward direction.
        //
        // @input n - Node (gradient op) whose contextinfo is to be searched,
        //        fwd_node - pointer to node from the forward pass that this node
        //        belongs to. fwd_node cannot be NULL.
        // @return Matching contextinfo in case a match is found; null otherwise.
        //         Also updates *fwd_node with pointer to forward node that this
        //         context matches.
        static const ContextInfo* SearchMatchingContext(const Node* n,
                                                        const Node** fwd_node);

        // Rewrites input node to a new node specified by its matching rewrite info.
        //
        // Method first searches matching rewrite info for input node and then
        // uses that info to rewrite.
        //
        // Input node may be deleted in case of rewrite. Attempt to use the node
        // after the call can result in undefined behaviors.
        //
        // @input  g - input graph, n - Node to be rewritten,
        //         ri - matching rewriteinfo
        // @return Status::OK(), if the input node is rewritten;
        //         Returns appropriate Status error code otherwise.
        //         Graph is updated in case the input node is rewritten.
        //         Otherwise, it is not updated.
        Status RewriteNode(std::unique_ptr<Graph>* g, Node* n, const RewriteInfo* ri);

        // Get nodes that will feed a list of TF tensors to the new
        // node that we are constructing.
        //
        // @input g - input graph,
        // @input inputs - inputs to old node that we are using for constructing
        //                 new inputs,
        // @input input_idx - the index in the 'inputs' vector pointing to the
        //                    current input that we have processed so far
        // @output input_idx - index will be incremented by the number of nodes
        //                     from 'inputs' that are processed
        // @input list_length - The expected length of list of TF tensors
        // @output output_nodes - the list of new nodes creating TF tensors
        //
        // @return None
        void GetNodesProducingTFTensorList(
            const TempArray<std::pair<Node*, int>>& inputs,
            int* input_idx, int list_length,
            std::vector<NodeBuilder::NodeOut>* output_nodes);

        // Get nodes that will feed a list of Mkldnn tensors to the new
        // node that we are constructing.
        //
        // @input g - input graph,
        // @input orig_node - Original node that we are rewriting
        // @input inputs - inputs to old node that we are using for constructing
        //                 new inputs,
        // @input input_idx - the index in the 'inputs' vector pointing to the
        //                    current input that we have processed so far
        // @output input_idx - index will be incremented by the number of nodes
        //                     from 'inputs' that are processed
        // @input list_length - The expected length of list of Mkldnn tensors
        // @output output_nodes - the list of new nodes creating Mkldnn tensors
        //
        // @return None
        void GetNodesProducingMkldnnTensorList(std::unique_ptr<Graph>* g,
                                               Node* orig_node, const TempArray<std::pair<Node*, int>>& inputs,
                                               int* input_idx, int list_length,
                                               std::vector<NodeBuilder::NodeOut>* output_nodes);

        // Get a node that will feed an Mkldnn tensor to the new
        // node that we are constructing. The output node could be (1) 'n'
        // if it is Mkldnn layer, or (2) a dummy node producing dummy Mkldnn tensor
        // if 'n' is not an Mkldnn layer.
        //
        // @input g - input graph,
        // @input orig_node - Original node that we are rewriting,
        // @input n - Node based on which we are creating Mkldnn node,
        // @input n_output_slot - the output slot of node 'n'
        //            which is feeding to the node that we are constructing
        // @output mkl_node - the new node that will feed Mkldnn tensor
        // @output mkl_node_output_slot - the slot number of mkl_node that
        //                                will feed the tensor
        // @return None
        void GetNodeProducingMkldnnTensor(std::unique_ptr<Graph>* g, Node* orig_node,
                                          Node* n, int n_output_slot, Node** mkl_node, int* mkl_node_output_slot);

        // Setup new inputs using old inputs 'inputs' for the rewritten node in 'nb'
        // in graph 'g'. Original node is input in 'old_node'. Inputs to 'nb' are
        // set up in contiguous fashion. 'workspace_tensors' carry graph nodes
        // producing workspace edges if 'are_workspace_tensors_available' is true.
        // Otherwise, 'workspace_tensors' is empty vector.
        //
        // For details, refer to 'Ordering of inputs after rewriting' section in the
        // documentation above.
        //
        // Returns Status::OK() if setting up inputs is successful, otherwise
        // returns appropriate status code.
        int SetUpContiguousInputs(
            std::unique_ptr<Graph>* g,
            const TempArray<std::pair<Node*, int>>& old_node_inputs,
            NodeBuilder* nb, Node* old_node,
            std::vector<NodeBuilder::NodeOut>* workspace_tensors,
            bool are_workspace_tensors_available);

        // Setup new inputs using old inputs 'inputs' for the rewritten node in 'nb'
        // in graph 'g'. Original node is input in 'orig_node'.
        //
        // For details, refer to 'Ordering of Tensorflow tensors and Mkldnn tensors'
        // section in the documentation above.
        //
        // Returns Status::OK() if setting up inputs is successful, otherwise
        // returns appropriate status code.
        Status SetUpInputs(std::unique_ptr<Graph>* g,
                           const TempArray<std::pair<Node*, int>>& inputs,
                           NodeBuilder* nb, Node* orig_node);

        // Add workspace edge on the input or output side of Node 'orig_node' by using
        // NodeBuilder 'nb' for the new node provided. If 'orig_node' does not dictate
        // adding workspace edge then do not add it. Workspace Tensorflow and Mkldnn
        // tensors, if they need to be added, will be set into these tensors.
        // If we set workspace tensors, then are_ws_tensors_added should be true.
        void AddWorkSpaceEdgeIfNeeded(std::unique_ptr<Graph>* g, Node* orig_node,
                                      NodeBuilder* nb,
                                      std::vector<NodeBuilder::NodeOut>* ws_tensors,
                                      bool* are_ws_tensors_added);

        // Generate a graph node in graph 'g' representing a dummy Mkldnn tensor node,
        // using node for original node 'orig_node' and return it in '*out'.
        // TODO: We should move this to mkldnn_util.h
        void GetDummyMkldnnTensorNode(std::unique_ptr<Graph>* g, Node** out,
                                      Node* orig_node);
        void GetDummyWorkspaceTensorNode(std::unique_ptr<Graph>* g, Node** out,
                                         Node* orig_node);

    private:
        bool MkldnnUseWeightsAsGiven_ = false; // Carry this flag from session_options
    };

    MkldnnLayoutRewritePass::ContextInfo
        MkldnnLayoutRewritePass::biasaddgrad_conv2dwithbias_context_;
    MkldnnLayoutRewritePass::ContextInfo
        MkldnnLayoutRewritePass::biasaddgrad_matmul_context_;
    std::vector<MkldnnLayoutRewritePass::ContextInfo*> MkldnnLayoutRewritePass::cinfo_;

    // We register Mkldnn rewrite pass for phase 1 in post partitioning group.
    // We register it here so that we get a complete picture of all users of Mkldnn
    // nodes. Do not change the ordering of the Mkldnn passes.
    const OptimizationPassRegistry::Grouping kMkldnnLayoutRewritePassGroup =
        OptimizationPassRegistry::POST_PARTITIONING;
    REGISTER_OPTIMIZATION(kMkldnnLayoutRewritePassGroup, 1, MkldnnLayoutRewritePass);

    //////////////////////////////////////////////////////////////////////////
    //           Helper functions for creating new node
    //////////////////////////////////////////////////////////////////////////

    static void FillInputs(const Node* n,
                           TempArray<Node*>* control_edges,
                           TempArray<std::pair<Node*, int>>* in) {
        control_edges->clear();
        for (const Edge* e : n->in_edges()) {
            if (e->IsControlEdge()) {
                control_edges->push_back(e->src());
            } else {
                (*in)[e->dst_input()] = std::make_pair(e->src(), e->src_output());
            }
        }
        std::sort(control_edges->begin(), control_edges->end());
        if (n->op_def().is_commutative()) {
            // For commutative inputs, we sort the input by the input Node*
            // to get a canonical ordering (so that add(a,b) and add(b, a) will
            // hash to the same value if is_commutative is true for 'add').
            std::sort(in->begin(), in->end());
        }
    }

    void MkldnnLayoutRewritePass::GetNodesProducingTFTensorList(
        const TempArray<std::pair<Node*, int>>& inputs, int* input_idx,
        int list_length, std::vector<NodeBuilder::NodeOut>* output_nodes) {
        CHECK_LT(*input_idx, inputs.size());
        CHECK_GT(list_length, 0);
        CHECK_NOTNULL(output_nodes);
        output_nodes->reserve(list_length);

        while (list_length != 0) {
            CHECK_GT(list_length, 0);
            CHECK_LT(*input_idx, inputs.size());
            Node* n = inputs[*input_idx].first;
            int slot = inputs[*input_idx].second;
            // If input node 'n' is just producing a single tensor at
            // output slot 'slot' then we just add that single node.
            output_nodes->push_back(NodeBuilder::NodeOut(n, slot));
            (*input_idx)++;
            list_length--;
        }
    }

    // TODO: We should move this to mkldnn_util.h.
    void MkldnnLayoutRewritePass::GetDummyMkldnnTensorNode(std::unique_ptr<Graph>* g,
                                                           Node** out, Node* orig_node) {
        // We use a tensor of shape {8} and value 0,0,0,0,0,0,0,0 to represent
        // dummy Mkldnn tensor. 8 = 2*size_t.
        const DataType dt = DataTypeToEnum<uint8>::v();
        TensorProto proto;
        proto.set_dtype(dt);
        uint8 zero[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        proto.set_tensor_content(const_cast<const void*>(static_cast<void*>(&zero)),
                                 8);
        TensorShape dummy_shape({8});
        dummy_shape.AsProto(proto.mutable_tensor_shape());
        TF_CHECK_OK(NodeBuilder((*g)->NewName("DMT"), "Const")
                        .Attr("value", proto)
                        .Attr("dtype", dt)
                        .Device(orig_node->def().device()) // We place this node on
                                                           // the same device as the
                                                           // device of the original
                                                           // node.
                        .Finalize(&**g, out));

        // If number of inputs to the original node is > 0, then we add
        // control dependency between 1st input (index 0) of the original node and
        // the dummy Mkldnn node. This is needed because control-flow ops such as Enter,
        // Merge, etc, require frame_name of the dummy Mkldnn node to be same as the
        // rewritten node. Adding control edge between 1st input of the original node
        // and the dummy Mkldnn node ensures that the dummy node is in the same frame
        // as the original node. Choosing 1st input is not necessary - any input of
        // the original node is fine because all the inputs of a node are always in
        // the same frame.
        if (orig_node->num_inputs() > 0) {
            Node* orig_input0 = nullptr;
            TF_CHECK_OK(orig_node->input_node(0,
                                              const_cast<const Node**>(&orig_input0)));
            CHECK_NOTNULL((*g)->AddControlEdge(orig_input0, *out));
        }

        (*out)->set_assigned_device_name(orig_node->assigned_device_name());
    }

    void MkldnnLayoutRewritePass::GetNodesProducingMkldnnTensorList(
        std::unique_ptr<Graph>* g,
        Node* orig_node,
        const TempArray<std::pair<Node*, int>>& inputs,
        int* input_idx, int list_length,
        std::vector<NodeBuilder::NodeOut>* output_nodes) {
        CHECK_LT(*input_idx, inputs.size());
        CHECK_GT(list_length, 0);
        CHECK_NOTNULL(output_nodes);
        output_nodes->reserve(list_length);

        while (list_length != 0) {
            CHECK_GT(list_length, 0);
            CHECK_LT(*input_idx, inputs.size());
            Node* n = inputs[*input_idx].first;
            int slot = inputs[*input_idx].second;
            // If 'n' is producing a single tensor, then create a single Mkldnn tensor
            // node.
            Node* mkl_node = nullptr;
            int mkl_node_output_slot = 0;
            GetNodeProducingMkldnnTensor(g, orig_node, n, slot, &mkl_node,
                                         &mkl_node_output_slot);
            output_nodes->push_back(NodeBuilder::NodeOut(mkl_node,
                                                         mkl_node_output_slot));
            (*input_idx)++;
            list_length--;
        }
    }

    // Get an input node that will feed Mkldnn tensor to the new
    // node that we are constructing. An input node could be (1) 'n'
    // if it is Mkldnn layer, or (2) a dummy node producing dummy Mkldnn tensor
    // if 'n' is not an Mkldnn layer.
    void MkldnnLayoutRewritePass::GetNodeProducingMkldnnTensor(
        std::unique_ptr<Graph>* g,
        Node* orig_node, Node* n,
        int n_output_slot,
        Node** mkl_node,
        int* mkl_node_output_slot) {

        CHECK_NOTNULL(n);
        CHECK_NOTNULL(mkl_node);
        CHECK_NOTNULL(mkl_node_output_slot);
        if (IsProducingMkldnnTensor(n)) {
            // If we have visited this node and rewritten it, then it will generate
            // an edge that will receive Mkldnn tensor from a node.
            // First, let's assert that this op is Mkldnn layer.
            DataType T;
            TF_CHECK_OK(GetNodeAttr(n->def(), "T", &T));
            // If this op has been rewritten, then its name must have been same as
            // Mkldnn op.
            CHECK_EQ(mkl_op_registry::IsMkldnnOp(n->type_string(), T), true);
            // output slot number for Mkldnn tensor would be N+slot number of TensorFlow
            // tensor, where N is total number of TensorFlow tensors.
            *mkl_node = n;
            *mkl_node_output_slot =
                GetTensorMetaDataIndex(n_output_slot, n->num_outputs());
        } else {
            // If we have not visited the node and rewritten it, then we need
            // to create a dummy node that will feed a dummy Mkldnn tensor to this node.
            // DummyMkldnnTensor node has no input and generates only 1 output
            // (dummy Mkldnn tensor) as output slot number 0.
            GetDummyMkldnnTensorNode(g, mkl_node, orig_node);
            CHECK_NOTNULL(*mkl_node);
            *mkl_node_output_slot = 0;
        }
    }

    int MkldnnLayoutRewritePass::SetUpContiguousInputs(
        std::unique_ptr<Graph>* g,
        const TempArray<std::pair<Node*, int>>& old_node_inputs,
        NodeBuilder* nb, Node* old_node,
        std::vector<NodeBuilder::NodeOut>* workspace_tensors,
        bool are_workspace_tensors_available) {
        CHECK_NOTNULL(workspace_tensors);
        CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);

        // Number of input slots to original op
        // Input slots are represented by .Input() calls in REGISTER_OP.
        int old_node_input_slots = old_node->op_def().input_arg_size();
        // Actual number of inputs can be greater than or equal to number
        // of Input slots because inputs of type list could be unfolded.
        CHECK_GE(old_node_inputs.size(), old_node_input_slots);
        int nn_slot_idx = 0; // slot index for inputs of new node

        // Let's copy all inputs (TF tensors) of original node to new node.
        int iidx = 0;
        for (int on_slot_idx = 0; on_slot_idx < old_node_input_slots; on_slot_idx++) {
            // An input slot could be a single tensor or a list. We need
            // to handle this case accordingly.
            CHECK_LT(iidx, old_node_inputs.size());
            const OpDef::ArgDef& arg = old_node->op_def().input_arg(on_slot_idx);
            if (ArgIsList(arg)) {
                std::vector<NodeBuilder::NodeOut> new_node_inputs;
                int N = GetTensorListLength(arg, old_node);
                GetNodesProducingTFTensorList(old_node_inputs, &iidx, N,
                                              &new_node_inputs);
                nb->Input(new_node_inputs);
                nn_slot_idx++;
            } else {
                nb->Input(old_node_inputs[iidx].first, old_node_inputs[iidx].second);
                iidx++;
                nn_slot_idx++;
            }
        }

        // If workspace tensors are available for this op and we are using
        // contiguous ordering then we need to add Tensorflow tensor for
        // workspace here because Tensorflow tensor for workspace is the
        // last tensor in the list of Tensorflow tensors.
        if (are_workspace_tensors_available) {
            CHECK_EQ(workspace_tensors->size(), 2);
            // Tensorflow tensor
            nb->Input((*workspace_tensors)[0].node, (*workspace_tensors)[0].index);
            nn_slot_idx++;
        }

        // Let's now setup all Mkldnn inputs to new node.
        // Number of Mkldnn inputs must be same as number of TF inputs.
        iidx = 0;
        for (int on_slot_idx = 0; on_slot_idx < old_node_input_slots; on_slot_idx++) {
            // An input slot could be a single tensor or a list. We need
            // to handle this case accordingly.
            CHECK_LT(iidx, old_node_inputs.size());
            const OpDef::ArgDef& arg = old_node->op_def().input_arg(on_slot_idx);
            if (ArgIsList(arg)) {
                std::vector<NodeBuilder::NodeOut> new_node_inputs;
                int N = GetTensorListLength(arg, old_node);
                GetNodesProducingMkldnnTensorList(g, old_node, old_node_inputs, &iidx,
                                                  N, &new_node_inputs);
                nb->Input(new_node_inputs);
                nn_slot_idx++;
            } else {
                Node* mkl_node = nullptr;
                int mkl_node_output_slot = 0;
                GetNodeProducingMkldnnTensor(g, old_node, old_node_inputs[iidx].first,
                                             old_node_inputs[iidx].second,
                                             &mkl_node, &mkl_node_output_slot);
                nb->Input(mkl_node, mkl_node_output_slot);
                iidx++;
                nn_slot_idx++;
            }
        }

        // If workspace tensors are available for this op and we are using
        // contiguous ordering then we need to add Mkldnn tensor for
        // workspace here because Mkldnn tensor for workspace is the
        // last tensor in the list of Mkldnn tensors.
        if (are_workspace_tensors_available) {
            CHECK_EQ(workspace_tensors->size(), 2);
            // Mkldnn tensor
            nb->Input((*workspace_tensors)[1].node, (*workspace_tensors)[1].index);
            nn_slot_idx++;
        }

        return nn_slot_idx;
    }

    Status MkldnnLayoutRewritePass::SetUpInputs(
        std::unique_ptr<Graph>* g,
        const TempArray<std::pair<Node*, int>>& old_node_inputs,
        NodeBuilder* nb, Node* old_node) {
        // Let's check if we need to add workspace tensors for this node.
        // We add workspace edge only for MaxPool, LRN and BatchNorm.
        std::vector<NodeBuilder::NodeOut> workspace_tensors;
        bool are_workspace_tensors_available = false;
        AddWorkSpaceEdgeIfNeeded(g, old_node, nb, &workspace_tensors,
                                 &are_workspace_tensors_available);

        int new_node_input_slots = 0;
        CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
        new_node_input_slots = SetUpContiguousInputs(
            g, old_node_inputs, nb, old_node, &workspace_tensors,
            are_workspace_tensors_available);

        // Sanity check
        int old_node_input_slots = old_node->op_def().input_arg_size();
        if (!are_workspace_tensors_available) {
            // If we are not adding workspace tensors for this op, then the total
            // number of input slots to the new node _must_ be 2 times the number
            // of input slots to the original node: N original Tensorflow tensors and
            // N for Mkldnn tensors corresponding to each Tensorflow tensors.
            CHECK_EQ(new_node_input_slots, old_node_input_slots * 2);
        } else {
            // If we are adding workspace tensors for this op, then the total
            // The total number of input slots to new node _must_ be 2 times the number
            // of input slots to the original node: N original Tensorflow tensors and
            // N for Mkldnn tensors corresponding to each Tensorflow tensors plus 2
            // (for workspace Tensorflow tensor and workspace Mkldnn tensor).
            CHECK_EQ(new_node_input_slots, old_node_input_slots * 2 + 2);
        }

        return Status::OK();
    }

    //////////////////////////////////////////////////////////////////////////
    //           Helper functions related to workspace pass
    //////////////////////////////////////////////////////////////////////////

    // TODO: We should move this to mkldnn_util.h.
    void MkldnnLayoutRewritePass::GetDummyWorkspaceTensorNode(
        std::unique_ptr<Graph>* g, Node** out, Node* orig_node) {
        // We use a tensor of shape {1} and value 0 to represent
        // dummy float tensor. We need this as a dummy workspace tensor.
        // Workspace tensor has type float.
        const DataType dt = DataTypeToEnum<float>::v();
        TensorProto proto;
        proto.set_dtype(dt);
        float zero[1] = {0};
        proto.set_tensor_content(const_cast<const void*>(static_cast<void*>(&zero)),
                                 4);
        TensorShape dummy_shape({1});
        dummy_shape.AsProto(proto.mutable_tensor_shape());
        TF_CHECK_OK(NodeBuilder((*g)->NewName("DMT"), "Const")
                        .Attr("value", proto)
                        .Attr("dtype", dt)
                        .Device(orig_node->def().device()) // We place this node on
                                                           // same the device as the
                                                           // device of the original
                                                           // node.
                        .Finalize(&**g, out));

        // If number of inputs to the original node is > 0, then we add
        // control dependency between 1st input (index 0) of the original node and
        // the dummy Mkldnn node. This is needed because control-flow ops such as Enter,
        // Merge, etc, require frame_name of the dummy Mkldnn node to be same as the
        // rewritten node. Adding control edge between 1st input of the original node
        // and the dummy Mkldnn node ensures that the dummy node is in the same frame
        // as the original node. Choosing 1st input is not necessary - any input of
        // the original node is fine because all the inputs of a node are always in
        // the same frame.
        if (orig_node->num_inputs() > 0) {
            Node* orig_input0 = nullptr;
            TF_CHECK_OK(orig_node->input_node(0,
                                              const_cast<const Node**>(&orig_input0)));
            CHECK_NOTNULL((*g)->AddControlEdge(orig_input0, *out));
        }

        (*out)->set_assigned_device_name(orig_node->assigned_device_name());
    }

    void MkldnnLayoutRewritePass::AddWorkSpaceEdgeIfNeeded(
        std::unique_ptr<Graph>* g, Node* orig_node, NodeBuilder* nb,
        std::vector<NodeBuilder::NodeOut>* ws_tensors, bool* are_ws_tensors_added) {
        bool workspace_edge_added = false; // Default initializer
        CHECK_NOTNULL(are_ws_tensors_added);
        *are_ws_tensors_added = false; // Default initializer

        DataType T;
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        for (auto ws : wsinfo_) {
            if (orig_node->type_string() == ws.fwd_op && mkl_op_registry::IsMkldnnOp(GetMkldnnOpName(orig_node->type_string()), T)) {
                // If this op is a fwd op, then we need to check if there is an
                // edge from this node's fwd_slot to bwdop's bwd_slot. If there is
                // an edge, then we just add an attribute on this node for setting
                // workspace_passed to true. We don't add actual workspace edge
                // in this node. Actual workspace edge gets added in the backward
                // op for this node.
                for (const Edge* e : orig_node->out_edges()) {
                    if (e->src_output() == ws.fwd_slot &&
                        e->dst()->type_string() == ws.bwd_op &&
                        e->dst_input() == ws.bwd_slot)
                    {
                        nb->Attr("workspace_enabled", true);
                        VLOG(1) << "MkldnnLayoutRewritePass: workspace_enabled for "
                                << orig_node->type_string();
                        workspace_edge_added = true;
                        // We found the edge that we were looking for, so break.
                        break;
                    }
                }

                if (!workspace_edge_added) {
                    // If we are here, then we did not find backward operator for this
                    // node.
                    nb->Attr("workspace_enabled", false);
                }
            } else if (orig_node->type_string() == ws.bwd_op &&
                       mkl_op_registry::IsMkldnnOp(GetMkldnnOpName(orig_node->type_string()), T)) {

                // If this op is a bwd op, then we need to add workspace edge and
                // it's Mkldnn tensor edge between its corresponding fwd op and this
                // op. Corresponding fwd op is specified in 'fwd_op' field of
                // workspace info. fwd_slot and bwd_slot in workspace info specify
                // an edge between which slots connect forward and backward op.
                // Once all these criteria match, we add a workspace edge between
                // ws_fwd_slot and ws_bwd_slot. Its corresponding Mkldnn tensor is
                // determined by interleaved/contiguous ordering. Function
                // DataIndexToMetaDataIndex tells us the location of Mkldnn tensor
                // from the location of the Tensorflow tensor.
                for (const Edge* e : orig_node->in_edges()) {
                    if (e->src_output() == ws.fwd_slot &&
                        // We would have rewritten the forward op, so we need to use
                        // GetMkldnnOpName call to get its Mkldnn name.
                        e->src()->type_string() == GetMkldnnOpName(ws.fwd_op) &&
                        e->dst_input() == ws.bwd_slot)
                    {
                        nb->Attr("workspace_enabled", true);
                        CHECK_NOTNULL(ws_tensors);
                        // Add workspace edge between fwd op and bwd op.
                        ws_tensors->push_back(NodeBuilder::NodeOut(e->src(), ws.ws_fwd_slot));
                        // Add Mkldnn tensor edge for workspace edge between fwd op and bwd op.
                        ws_tensors->push_back(NodeBuilder::NodeOut(
                            e->src(), DataIndexToMetaDataIndex(ws.ws_fwd_slot,
                                                               e->src()->num_outputs())));
                        *are_ws_tensors_added = true;
                        // In terms of input ordering, we add these calls to add Input
                        // here because workspace edge (and its Mkldnn tensor) is the last
                        // edge in the fwdop and bwdop. So all inputs before workspace
                        // tensor have been added by SetUpInputs function.
                        VLOG(1) << "MkldnnLayoutRewritePass: workspace_enabled for "
                                << orig_node->type_string();
                        workspace_edge_added = true;
                        // We found the edge that we were looking for, so break.
                        break;
                    }
                }

                // If we are here means we did not find fwd op that feeds to this
                // bwd op. So in this case, we need to generate dummy tensors for
                // workspace input and Mkldnn tensor for workspace, and set
                // workspace_enabled to false.
                if (!workspace_edge_added) {
                    nb->Attr("workspace_enabled", false);
                    Node* dmt_ws = nullptr;     // Dummy tensor for workspace
                    Node* dmt_mkl_ws = nullptr; // Dummy Mkldnn tensor for workspace
                    GetDummyWorkspaceTensorNode(g, &dmt_ws, orig_node);
                    GetDummyMkldnnTensorNode(g, &dmt_mkl_ws, orig_node);
                    CHECK_NOTNULL(dmt_ws);
                    CHECK_NOTNULL(dmt_mkl_ws);
                    CHECK_NOTNULL(ws_tensors);
                    // We add dummy tensor as workspace tensor.
                    ws_tensors->push_back(NodeBuilder::NodeOut(dmt_ws, 0));
                    // We add dummy tensor as Mkldnn tensor for workspace tensor.
                    ws_tensors->push_back(NodeBuilder::NodeOut(dmt_mkl_ws, 0));
                    *are_ws_tensors_added = true;
                    VLOG(1) << "MkldnnLayoutRewritePass: dummy workspace_enabled for "
                            << orig_node->type_string();
                }
            } else {
                // If this node does not match any workspace info, then we do not
                // do anything special for workspace propagation for it.
            }
        }
    }

    //////////////////////////////////////////////////////////////////////////
    // Op-specific functions to copy attributes from old node to new node
    //////////////////////////////////////////////////////////////////////////

    void CopyAttrsAdd(const Node* orig_node, NodeBuilder* nb) {
        DataType T;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));

        // Add attributes to new node.
        nb->Attr("T", T);
        // We must propagate data_format for use in MkldnnTf conversion
        const string& data_format = GuessDataFormat(orig_node);
        CHECK(data_format) << "\n\t...for " << orig_node->name() << Endl;
        nb->Attr("data_format", data_format);
    }

    void CopyAttrsConv2D(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        string data_format;
        string padding;
        std::vector<int32> strides;
        bool use_cudnn_on_gpu;
        int omp_threads;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "strides", &strides));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "padding", &padding));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "data_format", &data_format));
        TF_CHECK_OK(
            GetNodeAttr(orig_node->def(), "use_cudnn_on_gpu", &use_cudnn_on_gpu));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("strides", strides);
        nb->Attr("padding", padding);
        nb->Attr("data_format", data_format);
        nb->Attr("use_cudnn_on_gpu", use_cudnn_on_gpu);

        if (GetNodeAttr(orig_node->def(), "openmp_threads", &omp_threads).ok()) {
            nb->Attr("openmp_threads", omp_threads);
        }
    }

    void CopyAttrsBiasAddGrad(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        string data_format;
        std::vector<int32> strides;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "strides", &strides));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "data_format", &data_format));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("strides", strides);
        nb->Attr("data_format", data_format);
    }

    void CopyAttrsIdentity(const Node* orig_node, NodeBuilder* nb) {
        DataType T;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        // Add attributes to new node.
        nb->Attr("T", T);
    }

    void CopyAttrsLRN(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        int depth_radius;
        float bias;
        float alpha;
        float beta;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "depth_radius", &depth_radius));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "bias", &bias));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "alpha", &alpha));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "beta", &beta));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("depth_radius", depth_radius);
        nb->Attr("bias", bias);
        nb->Attr("alpha", alpha);
        nb->Attr("beta", beta);
    }

    void CopyAttrsPooling(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        string data_format;
        string padding;
        std::vector<int32> ksize, strides;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "ksize", &ksize));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "strides", &strides));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "padding", &padding));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "data_format", &data_format));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("ksize", ksize);
        nb->Attr("strides", strides);
        nb->Attr("padding", padding);
        nb->Attr("data_format", data_format);
    }

    void CopyAttrsRelu(const Node* orig_node, NodeBuilder* nb) {
        DataType T;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));

        // Add attributes to new node.
        nb->Attr("T", T);
        // We must propagate data_format for use in MkldnnTf conversion
        const string& data_format = GuessDataFormat(orig_node);
        CHECK(data_format) << "\n\t...for " << orig_node->name() << Endl;
        nb->Attr("data_format", data_format);
    }

    void CopyAttrsReshape(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        DataType Tshape;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "Tshape", &Tshape));
        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("Tshape", Tshape);
    }

    void CopyAttrsSplit(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        string data_format;
        int num_split;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "num_split", &num_split));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "data_format", &data_format));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("num_split", num_split);
        nb->Attr("data_format", data_format);
    }

    void CopyAttrsConcat(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        int N;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "N", &N));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("N", N);
        nb->Attr("data_format", GuessDataFormat(orig_node));
    }

    void CopyAttrsConcatV2(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        int N;
        DataType tidx;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "N", &N));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "Tidx", &tidx));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("N", N);
        nb->Attr("Tidx", tidx);
        nb->Attr("data_format", GuessDataFormat(orig_node));
    }

    void CopyAttrsFusedBatchNorm(const Node* orig_node, NodeBuilder* nb) {
        DataType T;
        float epsilon;
        string data_format;
        bool is_training;

        // Get all attributes from old node.
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &T));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "epsilon", &epsilon));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "data_format", &data_format));
        TF_CHECK_OK(GetNodeAttr(orig_node->def(), "is_training", &is_training));

        // Add attributes to new node.
        nb->Attr("T", T);
        nb->Attr("epsilon", epsilon);
        nb->Attr("data_format", data_format);
        nb->Attr("is_training", is_training);
    }

    string GuessDataFormat(const Node* node) {
      // Guess data_format to be set for node
      // We must propagate data_format for use in MkldnnTf conversion
      string data_format;
      if (GetNodeAttr(node->def(), "data_format", &data_format).ok()) {
        return data_format;
      }
      // If node does not provide data_format, attempt to get it from the predecessors.
      for (const Edge* e : node->in_edges()) {
        if (e->IsControlEdge()) continue;
        string tmp;
        if (GetNodeAttr(e->src()->def(), "data_format", &tmp).ok()) {
          if (data_format) {
            CHECK(tmp == data_format) << "Conflicting data_format for inputs to op " << node->name();
          } else {
            data_format = tmp;
          }
        }
      }
      return data_format;
    }

    //////////////////////////////////////////////////////////////////////////
    //           Helper functions related to node merge pass
    //////////////////////////////////////////////////////////////////////////

    static int CountOutEdges(const Node* node) {
        int ret = 0;
        for (const Edge* e : node->out_edges()) {
            if (!e->IsControlEdge())
                ret++;
        }
        return ret;
    }

    static Node* FindMissingInputNode(const Node* node, const Node* knownInput1) {
        Node* res = nullptr;
        for (const Edge* e : node->in_edges()) {
            if (e->IsControlEdge())
                continue;
            if (e->src() == knownInput1)
                continue;
            if (res)
                return nullptr; // more than one missing input
            res = e->src();
        }
        return res;
    }

    static void ReconnectControlEdges(std::unique_ptr<Graph>* g, const Node* old, Node* nue) {
        for (const Edge* e : old->out_edges()) {
            if (e && e->IsControlEdge() && e->dst()) {
                (*g)->AddControlEdge(nue, e->dst());
            }
        }
        for (const Edge* e : old->in_edges()) {
            if (e && e->IsControlEdge() && e->src()) {
                (*g)->AddControlEdge(e->src(), nue);
            }
        }
    }

    static DataType VerifySameAttr_T(const Node* node1, const Node* node2) {
        DataType T1, T2;
        bool ok = true;
        ok = ok && GetNodeAttr(node1->def(), "T", &T1).ok();
        ok = ok && GetNodeAttr(node2->def(), "T", &T2).ok();
        ok = ok && T1 == T2;
        return ok ? T1 : DT_INVALID;
    }

    template <typename T>
    static bool GetAttr_value(const Node* node, T* result) {
        Tensor t;
        if (!GetNodeAttr(node->def(), "value", &t).ok())
            return false;
        if (t.dtype() != DataTypeToEnum<T>::v())
            return false;
        if (t.NumElements() != 1)
            return false;
        *result = t.scalar<T>().data()[0];
        return true;
    }

    template <typename T>
    inline bool GetAttr(const string& name, const Node* node, T* result) {
        return GetNodeAttr(node->def(), name, result).ok();
    }

    typedef std::vector<const Edge*> EdgeList;
    typedef std::vector<Node*> NodeList;

    Node* MkldnnLayoutRewritePass::CheckForNodeMerge(const Node* a) const {
        // TODO: Add check for type of node similar to CheckForNodeRewrite
        // once we support BiasAddGrad as Mkldnn layer.

        // Search for all matching mergeinfo.
        // We allow more than one match for extensibility.
        std::vector<const MergeInfo*> matching_mi;
        for (auto mi = minfo_.cbegin(); mi != minfo_.cend(); ++mi) {
            if (a->type_string() == mi->succ) {
                matching_mi.push_back(&*mi);
            }
        }

        for (const MergeInfo* mi : matching_mi) {
            const int N_in = a->num_inputs();
            if (mi->op >= N_in) {
                continue;
            }

            // Get the control edges and input of node
            TempArray<Node*> a_control_edges;
            TempArray<std::pair<Node*, int>> a_in(N_in);
            FillInputs(a, &a_control_edges, &a_in);

            // Get operand op of the operator
            Node* b = nullptr;
            b = a_in[mi->op].first;
            if (b == nullptr || (b->type_string() != mi->pred)) {
                // NOTE: Should the first check be assert?
                continue;
            }

            const int B_in = b->num_inputs();
            TempArray<Node*> b_control_edges;
            TempArray<std::pair<Node*, int>> b_in(B_in);
            FillInputs(b, &b_control_edges, &b_in);

            if (a_control_edges != b_control_edges) {
                // This is not a reason to avoid merging these nodes,
                // because the merged node will inherit control dependences
                // of all original nodes.
                ;
            }
            // We found a match.
            return b;
        }

        return nullptr;
    }

    Status MkldnnLayoutRewritePass::MergeNode(
        std::unique_ptr<Graph>* g, Node* succ, Node* pred) {

        CHECK_NOTNULL(succ);
        CHECK_NOTNULL(pred);

        if (pred->type_string() == csinfo_.split && succ->type_string() == csinfo_.mkl_conv2d_with_bias) {
            return MergeNode_MkldnnConv2DWithBias_groups(g, succ, pred);
        }

        if (pred->type_string() == csinfo_.mkl_conv2d_with_bias && succ->type_string() == csinfo_.relu) {
            return MergeNode_MkldnnConv2DWithBias_relu(g, succ, pred);
        }

        if (pred->type_string() == csinfo_.mkl_conv2d_with_bias && succ->type_string() == csinfo_.mkl_relu) {
            return MergeNode_MkldnnConv2DWithBias_relu(g, succ, pred);
        }

        if (pred->type_string() == csinfo_.mkl_conv2d && succ->type_string() == csinfo_.bias_add) {
            return MergeNode_MkldnnConv2DWithBias(g, succ, pred);
        }

        if (pred->type_string() == csinfo_.mul && succ->type_string() == csinfo_.maximum) {
            return MergeNode_MkldnnRelu(g, succ, pred);
        }

        if (pred->type_string() == csinfo_.abs && succ->type_string() == csinfo_.mul) {
            return MergeNode_MkldnnRelu2(g, succ, pred);
        }

        return NOGO;
    }

    Status MkldnnLayoutRewritePass::MergeNode_MkldnnConv2DWithBias(
        std::unique_ptr<Graph>* g, Node* succ, Node* pred) {
        // Match pattern:
        //   res = BiasAdd(_MkldnnConv2D(X, W), B)
        // Replace with:
        //   res = _MkldnnConv2DWithBias(X, W, B)

        // 1. Get all attributes from input nodes.
        DataType T_pred, T_succ;
        string padding;
        std::vector<int32> strides;
        string data_format_pred, data_format_succ;
        bool use_cudnn_on_gnu;
        TF_CHECK_OK(GetNodeAttr(pred->def(), "T", &T_pred));
        TF_CHECK_OK(GetNodeAttr(succ->def(), "T", &T_succ));
        TF_CHECK_OK(GetNodeAttr(pred->def(), "padding", &padding));
        TF_CHECK_OK(GetNodeAttr(pred->def(), "strides", &strides));
        TF_CHECK_OK(GetNodeAttr(pred->def(), "data_format", &data_format_pred));
        TF_CHECK_OK(GetNodeAttr(succ->def(), "data_format", &data_format_succ));
        TF_CHECK_OK(
            GetNodeAttr(pred->def(), "use_cudnn_on_gpu", &use_cudnn_on_gnu));
        // We check to ensure that data formats of both succ and pred are same.
        // We expect them to be same, so we can enforce this as assert.
        // But assert can be too strict, so we enforce this as a check.
        // If the check fails, then we do not merge two nodes.
        // We also do same check for devices.
        if (data_format_pred != data_format_succ || T_pred != T_succ ||
            pred->assigned_device_name() != succ->assigned_device_name() ||
            pred->def().device() != succ->def().device())
        {
            return Status(error::Code::INVALID_ARGUMENT,
                          "data_format or T attribute or devices of Conv2D and "
                          "BiasAdd do not match. Will skip node merge optimization");
        }

        const int succ_num = succ->num_inputs();
        TempArray<Node*> succ_control_edges;
        TempArray<std::pair<Node*, int>> succ_in(succ_num);
        FillInputs(succ, &succ_control_edges, &succ_in);

        const int pred_num = pred->num_inputs();
        TempArray<Node*> pred_control_edges;
        TempArray<std::pair<Node*, int>> pred_in(pred_num);
        FillInputs(pred, &pred_control_edges, &pred_in);

        // We need to ensure that there is only 1 edge between Conv2D and AddBias.
        // Otherwise, merging is semantically incorrect.
        if (pred->out_edges().size() != 1) {
            return Status(error::Code::INVALID_ARGUMENT,
                          "Conv2D has multiple outputs."
                          "Will skip node merge optimization");
        }

        for (const Edge* e : pred->out_edges()) {
            if (e->dst() != succ) {
                return Status(error::Code::INVALID_ARGUMENT,
                              "Conv2D does not feed to BiasAdd."
                              "Will skip node merge optimization");
            }
        }

        // 2. Get inputs from both the nodes.
        // Find the 2 inputs from the conv and the bias from the add Bias.
        // Get operand 0, 1 of conv2D and their Mkldnn tensors.
        CHECK_EQ(pred->in_edges().size(), 4); // _MkldnnConv2D must have 4 inputs.
        // Get operand 1 of add_bias
        // BiasAdd must have 2 inputs: Conv, bias
        CHECK_EQ(succ->in_edges().size(), 2);
        Node* oper3_mkl = nullptr;                     // Mkldnn tensor corresponding to oper3
        int oper3_mkl_slot = 0;                        // For dummy MKL tensor node, output slot is 0.
        GetDummyMkldnnTensorNode(g, &oper3_mkl, pred); // Get dummy Mkldnn tensor node
        // as BiasAdd does not have Mkldnn tensor as input.
        CHECK_NOTNULL(oper3_mkl);

        // Build new node. Use the shortest name of the original node names.
        const string& new_name = succ->name().size() < pred->name().size() ? succ->name() : pred->name();
        NodeBuilder nb(new_name, csinfo_.mkl_conv2d_with_bias);
        CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
        nb.Input(pred_in[0].first, pred_in[0].second); // In1 of Conv2D
        // pred_in[1] will be Mkldnn tensor for In1 if we follow interleaved
        // ordering, and it will be 2nd Tensorflow tensor for Conv2D if
        // we follow contiguous ordering.
        nb.Input(pred_in[1].first, pred_in[1].second); // In2 of Conv2D
        nb.Input(succ_in[1].first, succ_in[1].second); // In2 of BiasAdd
        nb.Input(pred_in[2].first, pred_in[2].second); // Mkldnn for In1 of Conv2D
        nb.Input(pred_in[3].first, pred_in[3].second); // Mkldnn for In2 of Conv2D
        nb.Input(oper3_mkl, oper3_mkl_slot);           // Mkldnn for In2 of BiasAdd

        // Copy attributes from Conv2D to Conv2DWithBias.
        CopyAttrsConv2D(const_cast<const Node*>(pred), &nb);
        CopyOrSetAttr("use_weights_as_given", nb, pred, MkldnnUseWeightsAsGiven_);

        // Copy the device assigned to old node to new node.
        nb.Device(succ->def().device());

        // Create node.
        Node* new_node = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &new_node));

        // Set the Mkldnn layer label for this op.
        new_node->AddAttr("_kernel", mkl_op_registry::kMkldnnOpLabel);

        // Incoming data edges from 'pred' node and 'succ' node to new 'new_node'
        // node are already copied in BuildNode. We handle control edges now.
        for (const Edge* e : pred->in_edges()) {
            if (e->IsControlEdge()) {
                CHECK_NOTNULL((*g)->AddControlEdge(e->src(), new_node));
            }
        }
        for (const Edge* e : succ->in_edges()) {
            if (e->IsControlEdge()) {
                CHECK_NOTNULL((*g)->AddControlEdge(e->src(), new_node));
            }
        }

        // Incoming edges are fixed, we will fix the outgoing edges now.
        // First, we will fix outgoing control edges from 'pred' node.
        // We don't need to handle outgoing data edges from 'pred' node
        // because pred has only 1 output going to succ node (we enforced
        // this check for merge already).
        for (const Edge* e : pred->out_edges()) {
            if (e->IsControlEdge()) {
                CHECK_NOTNULL((*g)->AddControlEdge(new_node, e->dst()));
            }
        }

        // Second, we will fix outgoing control and data edges from 'succ' node.
        for (const Edge* e : succ->out_edges()) {
            if (e->IsControlEdge()) {
                CHECK_NOTNULL((*g)->AddControlEdge(new_node, e->dst()));
            } else {
                CHECK_NOTNULL((*g)->AddEdge(new_node, e->src_output(), e->dst(),
                                            e->dst_input()));
            }
        }

        // Copy device assigned to old node to new node.
        // It's ok to use pred or succ as we have enforced a check that
        // both have same device assigned.
        new_node->set_assigned_device_name(pred->assigned_device_name());

        VLOG(1) << "MergeNode_MkldnnConv2DWithBias: new node "
                << new_node->DebugString() << " from "
                << pred->DebugString() << " and " << succ->DebugString();

        // Finalizing MergeNode_MkldnnConv2DWithBias
        (*g)->RemoveNode(succ);
        (*g)->RemoveNode(pred);

        return Status::OK();
    }

    Status MkldnnLayoutRewritePass::MergeNode_MkldnnConv2DWithBias_relu(
        std::unique_ptr<Graph>* g, Node* succ, Node* pred) {
        // Match patterns:
        //   res = Relu(_MkldnnConv2dWithBias(X, W, B, leak=leak1, ...))
        // Or:
        //   res = _MkldnnRelu(_MkldnnConv2dWithBias(X, W, B, leak=leak1, ...), leak=leak2)
        // Replace with:
        //   res = _MkldnnConv2DWithBias(X, W, B, leak=leak1*leak2, ...)

        Node* conv = pred;
        Node* relu = succ;

        CHECK_EQ(conv->type_string(), csinfo_.mkl_conv2d_with_bias);
        bool relu_is_mkl = relu->type_string() == csinfo_.mkl_relu;
        if (!relu_is_mkl) {
            CHECK_EQ(relu->type_string(), csinfo_.relu);
        }

        // 1. Get all attributes from input nodes
        DataType T = VerifySameAttr_T(conv, relu);
        if (T == DT_INVALID)
            return NOGO;

        float leak1 = 0.0, leak2 = 0.0;
        TF_CHECK_OK(GetNodeAttr(conv->def(), "leak", &leak1));
        if (relu_is_mkl) {
            TF_CHECK_OK(GetNodeAttr(relu->def(), "leak", &leak2));
        }

        int groups;
        TF_CHECK_OK(GetNodeAttr(conv->def(), "groups", &groups));

        // 2. Check attribute/device consistency
        if (!(conv->assigned_device_name() == relu->assigned_device_name() && conv->def().device() == relu->def().device()))
            return NOGO;

        // 3. Check merge consistency (make sure merge is semantically correct)
        for (const Edge* e : conv->out_edges()) {
            if (e->IsControlEdge())
                continue;
            if (!(e->dst() == relu))
                return NOGO;
        }

        EdgeList relu_edges_in;
        TF_CHECK_OK(relu->input_edges(&relu_edges_in));
        if (!(relu_edges_in[0]->src() == conv))
            return NOGO;

        // 4. Build new node. Use the shortest name of the originial node names.
        CHECK(kTensorOrdering == MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
        const string& new_name = conv->name().size() < relu->name().size() ? conv->name() : relu->name();
        NodeBuilder nb(new_name, conv->type_string());
        CopyAttrsConv2D(conv, &nb);
        CopyOrSetAttr("use_weights_as_given", nb, conv, MkldnnUseWeightsAsGiven_);
        nb.Attr("leak", leak1 * leak2);
        nb.Attr("groups", groups);
        nb.Attr("_kernel", mkl_op_registry::kMkldnnOpLabel);
        nb.Device(conv->def().device());

        // 5. Connect input edges
        EdgeList conv_edges_in;
        TF_CHECK_OK(conv->input_edges(&conv_edges_in));
        for (int j : {0, 1, 2, 3, 4, 5}) {
            nb.Input(conv_edges_in[j]->src(), conv_edges_in[j]->src_output());
        }

        // 6. Create node
        Node* new_conv = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &new_conv));

        // 7. Connect output edges
        ReconnectControlEdges(g, conv, new_conv);
        ReconnectControlEdges(g, relu, new_conv);
        std::set<Node*> maybe_rm_nodes;
        for (const Edge* e : relu->out_edges()) {
            if (e->IsControlEdge())
                continue;
            CHECK_NOTNULL((*g)->AddEdge(new_conv, e->src_output(), e->dst(), e->dst_input()));
            if (!relu_is_mkl && IsMkldnnOp(e->dst(), T)) {
                int i = GetTensorMetaDataIndex(e->dst_input(), e->dst()->num_inputs());
                const Node* dmt;
                if (e->dst()->input_node(i, &dmt).ok()) {
                    maybe_rm_nodes.insert(const_cast<Node*>(dmt));
                }
                CHECK_NOTNULL((*g)->AddEdge(new_conv, 1, e->dst(), i));
            }
        }

        // 8. Assign new node on device and log it
        new_conv->set_assigned_device_name(conv->assigned_device_name());

        VLOG(1) << "MergeNode_MkldnnConv2DWithBias_relu: new node "
                << new_conv->DebugString() << " from "
                << conv->type_string() << "[" << conv->name() << "] and "
                << relu->type_string() << "[" << relu->name() << "]";

        // 9. Remove old nodes
        // Finalizing MergeNode_MkldnnConv2DWithBias_relu
        (*g)->RemoveNode(conv);
        (*g)->RemoveNode(relu);
        for (Node* n : maybe_rm_nodes) {
            if (CountOutEdges(n) < 2) {
                (*g)->RemoveNode(n);
            }
        }

        return Status::OK();
    }

    Status MkldnnLayoutRewritePass::MergeNode_MkldnnConv2DWithBias_groups(
        std::unique_ptr<Graph>* g, Node* succ, Node* pred) {
        CHECK_EQ(pred->type_string(), csinfo_.split);
        CHECK_EQ(succ->type_string(), csinfo_.mkl_conv2d_with_bias);

        // Expect:
        //   const_filter = Const(shape=[groups, H, W, I, O/groups])
        //   const_bias = Const(shape=[groups, O/groups])
        //   X = some_tensor(shape=[..., C, ...])
        //   split_axis == concat_axis == dimension-of-C
        // Match this pattern:
        //   split = Split(split_axis, X, num_split=groups)
        //   unpack_filter = Unpack(const_filter, axis=0, num=groups)
        //   unpack_bias = Unpack(const_bias, axis=0, num=groups)
        //   conv = [_MkldnnConv2DWithBias(split[g], unpack_filter[g], unpack_bias[g])
        //            for g in range(groups)]
        //   concat = ConcatV2(conv, concat_axis) or _MkldnnConcatV2(conv, concat_axis)
        // Replace it with:
        //   res = _MkldnnConv2DWithBias(X, const_filter, const_bias, groups=groups)

        Node* X;
        Node* const_filter = nullptr;
        Node* const_bias = nullptr;

        Node* split = pred;
        Node* unpack_filter = nullptr;
        Node* unpack_bias = nullptr;
        NodeList conv;
        Node* concat = nullptr;

        std::set<Node*> maybe_rm_nodes;

        int split_axis = -1;
        int groups = -1;

        // Check `split` node
        EdgeList split_edges_in;
        TF_CHECK_OK(split->input_edges(&split_edges_in));
        if (!GetAttr("num_split", split, &groups))
            return NOGO;
        if (!GetAttr_value(split_edges_in[0]->src(), &split_axis))
            return NOGO;
        X = split_edges_in[1]->src();
        conv.resize(groups, nullptr);
        for (const Edge* e : split->out_edges()) {
            if (e->IsControlEdge())
                continue;
            if (!(e->dst()->type_string() == csinfo_.mkl_conv2d_with_bias))
                return NOGO;
            if (size_t(e->src_output()) >= conv.size())
                return NOGO;
            if (conv[e->src_output()])
                return NOGO; // not the pattern we are matching
            conv[e->src_output()] = e->dst();
        }

        if (std::any_of(conv.begin(), conv.end(), [](const Node* n) { return n == nullptr; }))
            return NOGO;

        // Now check `conv` nodes
        for (int i = 0; i < conv.size(); ++i) {
            EdgeList conv_edges_in;
            TF_CHECK_OK(conv[i]->input_edges(&conv_edges_in));

            // in edges
            const EdgeList& in = conv_edges_in;

            if (!unpack_filter) {
                unpack_filter = in[1]->src();
                if (!(unpack_filter->type_string() == csinfo_.unpack))
                    return NOGO;
            }
            if (!(in[1]->src() == unpack_filter && in[1]->src_output() == i))
                return NOGO;

            if (!unpack_bias) {
                unpack_bias = in[2]->src();
                if (!(unpack_bias->type_string() == csinfo_.unpack))
                    return NOGO;
            }
            if (!(in[2]->src() == unpack_bias && in[2]->src_output() == i))
                return NOGO;

            for (int j : {3, 4, 5}) {
                if (in[j]->src())
                    maybe_rm_nodes.insert(in[j]->src());
            }

            // out edges of conv[i]
            for (const Edge* e : conv[i]->out_edges()) {
                if (e->IsControlEdge())
                    continue;
                if (e->src_output() != 0)
                    continue;
                if (!concat) {
                    concat = e->dst();
                    bool concat_type_good = concat && (
                      concat->type_string() == csinfo_.concatv2 ||
                      concat->type_string() == GetMkldnnOpName(csinfo_.concatv2));
                    if (!concat_type_good) return NOGO;
                }
                if (!(e->dst() == concat && e->dst_input() == i))
                    return NOGO;
            }
        }

        // Now check 'unpack_filter' and 'unpack_bias' nodes
        EdgeList unpack_filter_edges_in;
        TF_CHECK_OK(unpack_filter->input_edges(&unpack_filter_edges_in));

        EdgeList unpack_bias_edges_in;
        TF_CHECK_OK(unpack_bias->input_edges(&unpack_bias_edges_in));

        int unpack_filter_axis = -1, unpack_filter_num = -1;
        int unpack_bias_axis = -1, unpack_bias_num = -1;

        if (!GetAttr("axis", unpack_filter, &unpack_filter_axis))
            return NOGO;
        if (!GetAttr("num", unpack_filter, &unpack_filter_num))
            return NOGO;
        const_filter = unpack_filter_edges_in[0]->src();
        if (!IsConstOrImmutableConst(const_filter)) return NOGO;

        if (!GetAttr("axis", unpack_bias, &unpack_bias_axis))
            return NOGO;
        if (!GetAttr("num", unpack_bias, &unpack_bias_num))
            return NOGO;
        const_bias = unpack_bias_edges_in[0]->src();
        if (!IsConstOrImmutableConst(const_bias)) return NOGO;

        if (!(unpack_filter_axis == 0 && unpack_bias_axis == 0))
            return NOGO;
        if (!(unpack_filter_num == groups && unpack_bias_num == groups))
            return NOGO;

        // Now check `concat` node
        EdgeList concat_edges_in;
        TF_CHECK_OK(concat->input_edges(&concat_edges_in));
        CHECK_GT(concat_edges_in.size(), groups);
        const Edge* concat_axis_edge = concat_edges_in[groups];

        int concat_axis = -1;
        if (!(GetAttr_value(concat_axis_edge->src(), &concat_axis)))
            return NOGO;

        if (!(concat_axis == split_axis))
            return NOGO;

        // 1. Define all attributes for new node
        DataType T = VerifySameAttr_T(split, concat);
        if (T == DT_INVALID)
            return NOGO;

        float leak = 0.0;
        TF_CHECK_OK(GetNodeAttr(conv[0]->def(), "leak", &leak));
        // TODO: check attrs are same for conv, e.g. leak is same

        // 2. Check attribute/device consistency
        const auto& device_name = X->assigned_device_name();
        {
            bool ok = true;
            ok = ok && device_name == concat->assigned_device_name();
            ok = ok && device_name == const_filter->assigned_device_name();
            ok = ok && device_name == const_bias->assigned_device_name();
            if (!ok)
                return NOGO;
        }
        const auto& device = X->def().device();
        {
            bool ok = true;
            ok = ok && device == concat->def().device();
            ok = ok && device == const_filter->def().device();
            ok = ok && device == const_bias->def().device();
            if (!ok)
                return NOGO;
        }

        // 3. Check merge consistency (make sure merge is semantically correct)
        // TODO: verify. This should be done above

        // 4. Build new node. Use name of conv[0].
        CHECK(kTensorOrdering == MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
        NodeBuilder nb(conv[0]->name(), conv[0]->type_string());
        CopyAttrsConv2D(conv[0], &nb);
        CopyOrSetAttr("use_weights_as_given", nb, conv[0], MkldnnUseWeightsAsGiven_);
        nb.Attr("leak", leak);
        nb.Attr("_kernel", mkl_op_registry::kMkldnnOpLabel);
        nb.Attr("groups", groups);
        nb.Device(device);

        // 5. Connect input edges
        nb.Input(split_edges_in[1]->src(), split_edges_in[1]->src_output());
        nb.Input(const_filter, 0);
        nb.Input(const_bias, 0);
        {
            Node* dmt;
            int dmt_slot;
            GetNodeProducingMkldnnTensor(g, split,
                                         split_edges_in[1]->src(), split_edges_in[1]->src_output(),
                                         &dmt, &dmt_slot);
            nb.Input(dmt, dmt_slot);
        }
        {
            Node* dmt;
            int dmt_slot;
            GetNodeProducingMkldnnTensor(g, split, const_filter, 0, &dmt, &dmt_slot);
            nb.Input(dmt, dmt_slot);
        }
        {
            Node* dmt;
            int dmt_slot;
            GetNodeProducingMkldnnTensor(g, split, const_bias, 0, &dmt, &dmt_slot);
            nb.Input(dmt, dmt_slot);
        }

        // 6. Create node
        Node* res = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &res));

        // 7. Connect output edges and control edges
        ReconnectControlEdges(g, split, res);
        for (Node* c : conv) {
            ReconnectControlEdges(g, c, res);
        }
        ReconnectControlEdges(g, concat, res);
        for (const Edge* e : concat->out_edges()) {
            if (e->IsControlEdge())
                continue;
            CHECK_NOTNULL((*g)->AddEdge(res, e->src_output(), e->dst(), e->dst_input()));
            if (!IsMkldnnOp(concat, T) && IsMkldnnOp(e->dst(), T)) {
                int i = GetTensorMetaDataIndex(e->dst_input(), e->dst()->num_inputs());
                const Node* dmt;
                TF_CHECK_OK(e->dst()->input_node(i, &dmt)) << e->dst()->DebugString();
                maybe_rm_nodes.insert(const_cast<Node*>(dmt));
                CHECK_NOTNULL((*g)->AddEdge(res, 1, e->dst(), i));
            }
        }

        // 8. Assign new node on device
        res->set_assigned_device_name(device_name);

        // 9. Remove old nodes
        // Finalizing MergeNode_MkldnnConv2DWithBias_groups
        (*g)->RemoveNode(split);
        (*g)->RemoveNode(unpack_filter);
        (*g)->RemoveNode(unpack_bias);
        for (Node* c : conv) {
            (*g)->RemoveNode(c);
        }
        (*g)->RemoveNode(concat);
        for (Node* n : maybe_rm_nodes) { // One pass is enough
            if (n->id() >= 0 && CountOutEdges(n) < 2) {
                (*g)->RemoveNode(n);
            }
        }

        VLOG(1) << "MergeNode_MkldnnConv2DWithBias_groups: new node "
                << res->DebugString();

        return Status::OK();
    }

    Status MkldnnLayoutRewritePass::MergeNode_MkldnnRelu(
        std::unique_ptr<Graph>* g, Node* succ, Node* pred) {
        CHECK(pred->type_string() == csinfo_.mul);
        CHECK(succ->type_string() == csinfo_.maximum);

        // Match pattern:
        //   res = Maximum(X, Mul(X, Const/leak))
        // Replace with:
        //   res = _MkldnnRelu(X, DMT|X)

        Node* node_max = succ;
        Node* node_mul = pred;
        Node* node_X = nullptr;
        Node* node_leak = nullptr;
        std::set<Node*> maybe_rm_nodes;

        DataType T = VerifySameAttr_T(node_max, node_mul);
        if (T == DT_INVALID)
            return NOGO;

        if (!(node_X = FindMissingInputNode(node_max, node_mul)))
            return NOGO;
        if (!(node_leak = FindMissingInputNode(node_mul, node_X)))
            return NOGO;
        if (!IsConstOrImmutableConst(node_leak))
            return NOGO;

        float leak;
        if (!GetAttr_value(node_leak, &leak))
            return NOGO;
        if (!(0 <= leak && leak < 1.0))
            return NOGO;

        string data_format = GuessDataFormat(node_X);
        if (data_format.empty())
            return NOGO;

        // Make sure node_mul has only one feedee (= can be merged with node_max)
        if (node_mul->out_edges().size() != 1)
            return NOGO;

        NodeBuilder::NodeOut node_X_meta;
        if (IsMkldnnOp(node_X, T)) {
            int i = GetTensorMetaDataIndex(0, node_X->num_outputs());
            node_X_meta = NodeBuilder::NodeOut(node_X, i);
        } else {
            Node* meta = nullptr;
            GetDummyMkldnnTensorNode(g, &meta, node_X);
            node_X_meta = NodeBuilder::NodeOut(meta, 0);
        }

        // Build the new node, using name of node_max
        NodeBuilder nb(node_max->name(), csinfo_.mkl_relu);
        CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
        nb.Input(node_X, 0);
        nb.Input(node_X_meta);
        nb.Device(node_max->def().device());
        nb.Attr("_kernel", mkl_op_registry::kMkldnnOpLabel);
        nb.Attr("T", T);
        nb.Attr("leak", leak);
        nb.Attr("data_format", data_format);

        Node* new_node = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &new_node));

        // Connect outputs of the new node
        ReconnectControlEdges(g, node_max, new_node);
        ReconnectControlEdges(g, node_mul, new_node);
        ReconnectControlEdges(g, node_leak, new_node);
        for (const Edge* e : node_max->out_edges()) {
            if (e->IsControlEdge())
                continue;
            CHECK_NOTNULL((*g)->AddEdge(new_node, e->src_output(), e->dst(), e->dst_input()));
            if (IsMkldnnOp(e->dst(), T)) {
                int i = GetTensorMetaDataIndex(e->dst_input(), e->dst()->num_inputs());
                const Node* dmt;
                if (e->dst()->input_node(i, &dmt).ok()) {
                    maybe_rm_nodes.insert(const_cast<Node*>(dmt));
                }
                CHECK_NOTNULL((*g)->AddEdge(new_node, 1, e->dst(), i));
            }
        }

        new_node->set_assigned_device_name(node_max->assigned_device_name());

        VLOG(1) << "MergeNode_MkldnnRelu: new node "
                << new_node->DebugString() << " from "
                << node_max->name() << "("
                << node_X->name() << "," << node_mul->name() << "("
                << node_X->name() << "," << node_leak->name() << "))";

        // Finalizing MergeNode_MkldnnRelu
        (*g)->RemoveNode(node_max);
        (*g)->RemoveNode(node_mul);
        if (CountOutEdges(node_leak) < 1)
            (*g)->RemoveNode(node_leak);
        for (Node* n : maybe_rm_nodes) {
            if (CountOutEdges(n) < 2) {
                (*g)->RemoveNode(n);
            }
        }

        return Status::OK();
    }

    Status MkldnnLayoutRewritePass::MergeNode_MkldnnRelu2(
        std::unique_ptr<Graph>* g, Node* succ, Node* pred) {
        CHECK(pred->type_string() == csinfo_.abs);
        CHECK(succ->type_string() == csinfo_.mul);

        // Match any of patterns:
        //   res = Add(Mul(Abs(X), A), Mul(X, 1 - A))
        //   res = Add(Mul(A, Abs(X)), Mul(X, 1 - A))
        //   res = Add(Mul(X, 1 - A), Mul(Abs(X), A))
        //   res = Add(Mul(X, 1 - A), Mul(A, Abs(X)))
        // Replace with:
        //   res = _MkldnnRelu(X, DMT|X, 1 - 2*A)

        Node* node_x = nullptr;
        Node* node_abs = pred; // Abs(X)
        Node* node_f1 = succ; // Mul(Abs(X), A)
        Node* node_f2 = nullptr; // Mul(X, 1 - A)
        Node* node_a = nullptr;
        Node* node_1a = nullptr; // 1 - A
        Node* node_add = nullptr;

        DataType T = VerifySameAttr_T(node_abs, node_f1);
        if (T == DT_INVALID) return NOGO;

        // 1. Match the pattern - associate all the node_* variables, discover leak
        float leak = 0.0;
        {
          for (const Edge* e : node_abs->in_edges()) {
            if (e->IsControlEdge()) continue;
            if (node_x != nullptr) return NOGO;
            node_x = e->src();
          }
          if (!node_x) return NOGO;

          for (const Edge* e : node_abs->out_edges()) {
            if (e->IsControlEdge()) continue;
            if (e->dst() != node_f1) return NOGO;
          }

          for (const Edge* e : node_f1->in_edges()) {
            if (e->IsControlEdge()) continue;
            if (e->src() == node_abs) continue;
            if (node_a != nullptr) return NOGO;
            node_a = e->src();
            if (node_a->type_string() != csinfo_.constant) return NOGO;
          }
          if (!node_a) return NOGO;
          float value_a;
          if (!GetAttr_value(node_a, &value_a)) return NOGO;

          for (const Edge* e : node_f1->out_edges()) {
            if (e->IsControlEdge()) continue;
            if (node_add != nullptr) return NOGO;
            node_add = e->dst();
          }
          if (!node_add) return NOGO;

          for (const Edge* e : node_add->in_edges()) {
            if (e->IsControlEdge()) continue;
            if (e->src() == node_f1) continue;
            if (node_f2 != nullptr) return NOGO;
            node_f2 = e->src();
          }
          if (!node_f2) return NOGO;

          for (const Edge* e : node_f2->in_edges()) {
            if (e->IsControlEdge()) continue;
            if (e->src() == node_x) continue;
            if (node_1a != nullptr) return NOGO;
            node_1a = e->src();
            if (node_1a->type_string() != csinfo_.constant) return NOGO;
          }
          if (!node_1a) return NOGO;
          float value_1a;
          if (!GetAttr_value(node_1a, &value_1a)) return NOGO;
          if (std::abs(value_a - 1.0 + value_1a) > std::numeric_limits<float>::epsilon()) return NOGO;

          for (const Edge* e : node_f2->out_edges()) {
            if (e->IsControlEdge()) continue;
            if (e->dst() == node_add) continue;
            return NOGO;
          }

          double leak1 = 1.0 - 2.0 * value_a;
          double leak2 = 2.0 * value_1a - 1.0;
          leak = 0.5 * (leak1 + leak2);
        }

        // 5a. Guess data_format of the input
        string data_format = GuessDataFormat(node_x);
        if (data_format.empty()) return NOGO;

        // 5b. Identify/create node_x_meta for new node
        NodeBuilder::NodeOut node_x_meta;
        {
          if (IsMkldnnOp(node_x, T)) {
              int i = GetTensorMetaDataIndex(0, node_x->num_outputs());
              node_x_meta = NodeBuilder::NodeOut(node_x, i);
          } else {
              Node* meta = nullptr;
              GetDummyMkldnnTensorNode(g, &meta, node_x);
              node_x_meta = NodeBuilder::NodeOut(meta, 0);
          }
        }

        // 5. Build new node, using name of node_add
        NodeBuilder nb(node_add->name(), csinfo_.mkl_relu);
        {
          CHECK_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
          nb.Input(node_x, 0);
          nb.Input(node_x_meta);

          nb.Device(node_add->def().device());
          nb.Attr("_kernel", mkl_op_registry::kMkldnnOpLabel);
          nb.Attr("T", T);
          nb.Attr("leak", leak);
          nb.Attr("data_format", data_format);
        }

        // 6. Commit the new node into the graph
        Node* new_node = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &new_node));

        new_node->set_assigned_device_name(node_add->assigned_device_name());

        // 7. Connect the new node to the rest of the graph
        std::set<Node*> maybe_rm_nodes;
        {
          ReconnectControlEdges(g, node_abs, new_node);
          ReconnectControlEdges(g, node_f1, new_node);
          ReconnectControlEdges(g, node_f2, new_node);
          ReconnectControlEdges(g, node_a, new_node);
          ReconnectControlEdges(g, node_1a, new_node);
          ReconnectControlEdges(g, node_add, new_node);
          for (const Edge* e : node_add->out_edges()) {
            if (e->IsControlEdge()) {
              continue;
            }
            CHECK_NOTNULL((*g)->AddEdge(new_node, e->src_output(), e->dst(), e->dst_input()));
            if (IsMkldnnOp(e->dst(), T)) {
              int i = GetTensorMetaDataIndex(e->dst_input(), e->dst()->num_inputs());
              Node* dmt;
              TF_CHECK_OK(e->dst()->input_node(i, &dmt));
              maybe_rm_nodes.insert(dmt);
              CHECK((*g)->AddEdge(new_node, 1, e->dst(), i));
            }
          }
        }

        VLOG(1) << "MergeNode_MkldnnRelu2: new node "
                << new_node->DebugString() << " from "
                << node_add->name() << "(...)";

        // 8. Cleanup
        // Finalize MergeNode_MkldnnRelu2
        maybe_rm_nodes.insert(node_a);
        maybe_rm_nodes.insert(node_1a);
        for (Node* n : maybe_rm_nodes) {
          if (CountOutEdges(n) < 2) {
            (*g)->RemoveNode(n);
          }
        }
        (*g)->RemoveNode(node_abs);
        (*g)->RemoveNode(node_f1);
        (*g)->RemoveNode(node_f2);
        (*g)->RemoveNode(node_add);

        return Status::OK();
    }

    Status MkldnnLayoutRewritePass::InsertUndoWeights(
        std::unique_ptr<Graph>* g, Node* conv) {
        CHECK(conv->type_string() == csinfo_.conv2d);

        // Match pattern:
        //   Conv2D(_, W, _)
        // Rewrite to:
        //   Conv2D(_, _UndoWeightsMkldnn(W), _)

        // Locate the weights edge
        const Edge* edge_w = nullptr;
        for (const Edge* e : conv->in_edges()) {
            if (e->IsControlEdge()) continue;
            if (e->dst_input() == 1) {
                CHECK(edge_w == nullptr);
                edge_w = e;
            }
        }
        CHECK(edge_w);
        if (edge_w->src()->type_string() == csinfo_.undo_weights) {
            return Status(error::Code::ALREADY_EXISTS, {});
        }

        DataType T = VerifySameAttr_T(conv, edge_w->src());
        if (T != DT_FLOAT) return NOGO;

        NodeBuilder nb(conv->name() + "_U", csinfo_.undo_weights);
        nb.Input(edge_w->src(), edge_w->src_output());
        nb.Device(edge_w->src()->def().device());
        nb.Attr("T", T);

        // Commit the new node into the graph
        Node* node_undoweights = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &node_undoweights));
        node_undoweights->set_assigned_device_name(conv->assigned_device_name());

        // Connect the output of the new node
        TF_CHECK_OK((*g)->UpdateEdge(node_undoweights, 0, conv, 1));

        return Status::OK();
    }

    //////////////////////////////////////////////////////////////////////////
    //           Helper functions for node rewrite
    //////////////////////////////////////////////////////////////////////////

    Status MkldnnLayoutRewritePass::RewriteNode(
        std::unique_ptr<Graph>* g, Node* orig_node, const RewriteInfo* ri) {
        CHECK_NOTNULL(ri);
        CHECK_NOTNULL(orig_node);

        VLOG(1) << "MkldnnLayoutRewritePass: Original node:" << orig_node->DebugString();

        // Check if this is scenario 2 (context-based rewrite).
        // Get the matching ContextInfo if it is.
        const Node* fwd_node = nullptr;
        const ContextInfo* ci = nullptr;
        bool is_context_based_rewrite = false;
        if ((ci = SearchMatchingContext(orig_node, &fwd_node)) != nullptr) {
            CHECK_NOTNULL(fwd_node);
            is_context_based_rewrite = true;

            // Sanity checks for context-based rewrite (if any)
            if (orig_node->type_string() == csinfo_.bias_add_grad && ri->new_name == csinfo_.mkl_conv2d_with_bias_backprop_bias) {
                DataType orig_T, ctx_T;
                string orig_data_format, ctx_data_format;
                TF_CHECK_OK(GetNodeAttr(orig_node->def(), "T", &orig_T));
                TF_CHECK_OK(
                    GetNodeAttr(orig_node->def(), "data_format", &orig_data_format));
                TF_CHECK_OK(GetNodeAttr(fwd_node->def(), "T", &ctx_T));
                TF_CHECK_OK(
                    GetNodeAttr(fwd_node->def(), "data_format", &ctx_data_format));

                if (orig_data_format != ctx_data_format || orig_T != ctx_T ||
                    orig_node->assigned_device_name() !=
                        fwd_node->assigned_device_name() ||
                    orig_node->def().device() != fwd_node->def().device())
                {
                    return Status(
                        error::Code::INVALID_ARGUMENT,
                        "data_format or T attribute or devices of BiasAddGrad and "
                        "Conv2D do not match. Will skip node rewrite optimization");
                }
            } else if (orig_node->type_string() == csinfo_.bias_add_grad && ri->new_name == csinfo_.matmul) {
                // When BiasAddGrad has MatMul in context, we do not do any rewrite
                // and leave BiasAddGrad as it is. But we check for this condition
                // when we check for node rewrite rule. So we should not even come
                // here for MatMul. So we will fail now.
                return Status(
                    error::Code::INVALID_ARGUMENT,
                    "No rewrite is required for BiasAddGrad for MatMul context.");
            }
        }

        // Get all inputs.
        const int num_inputs = orig_node->in_edges().size();
        TempArray<Node*> control_edges;
        TempArray<std::pair<Node*, int>> inputs(num_inputs);
        FillInputs(orig_node, &control_edges, &inputs);

        // Build new node. Use same name as original node, but change the op name.
        NodeBuilder nb(orig_node->name(), ri->new_name);
        // Copy user-specified device assigned to original node to new node.
        nb.Device(orig_node->def().device());
        // Set up new inputs to the rewritten node.
        Status s = SetUpInputs(g, inputs, &nb, orig_node);
        if (s != Status::OK()) {
            return s;
        }

        // Copy attributes from original node to new node (for scenario 1).
        // For context-based rewrite, we use context to copy the attributes.
        if (is_context_based_rewrite) {
            if (orig_node->type_string() == csinfo_.bias_add_grad && ri->new_name == csinfo_.mkl_conv2d_with_bias_backprop_bias) {
                CHECK_NOTNULL(fwd_node);
                ri->copy_attrs(fwd_node, &nb);
            } else {
                return Status(error::Code::UNIMPLEMENTED,
                              "Unimplemented case for node rewrite optimization.");
            }
        } else {
            ri->copy_attrs(const_cast<const Node*>(orig_node), &nb);
        }
        // Set the Mkldnn layer label for this op.
        nb.Attr("_kernel", mkl_op_registry::kMkldnnOpLabel);
        if (nb.op_def().name() == GetMkldnnOpName(csinfo_.conv2d)) {
            CopyOrSetAttr("use_weights_as_given", nb, orig_node, MkldnnUseWeightsAsGiven_);
        }

        // Finalize graph and get new node.
        Node* new_node = nullptr;
        TF_CHECK_OK(nb.Finalize(&**g, &new_node));

        // Incoming data edges from 'orig_node' node to new 'new_node' node are
        // already copied in BuildNode. We need to handle control edges now.
        for (const Edge* e : orig_node->in_edges()) {
            if (e->IsControlEdge()) {
                CHECK_NOTNULL((*g)->AddControlEdge(e->src(), new_node));
            }
        }

        // Copy outgoing edges from 'orig_node' node to new
        // 'new_node' node, since the output also follows same ordering among
        // Tensorflow tensors and Mkldnn tensors. We need to connect Tensorflow
        // tensors appropriately. Specifically, nth output of the original node
        // will become 2*nth output of the Mkldnn node for the interleaved ordering
        // of the tensors. For the contiguous ordering of the tensors, it will be n.
        // GetTensorDataIndex provides this mapping function.
        for (const Edge* e : orig_node->out_edges()) {
            if (e->IsControlEdge()) {
                CHECK_NOTNULL((*g)->AddControlEdge(new_node, e->dst()));
            } else {
                CHECK_NOTNULL((*g)->AddEdge(new_node, GetTensorDataIndex(e->src_output(), e->src()->num_outputs()),
                                            e->dst(), e->dst_input()));
            }
        }

        // Copy the runtime device assigned from original code to new node.
        new_node->set_assigned_device_name(orig_node->assigned_device_name());

        // Delete original node and mark new node as rewritten.
        // Finalizing RewriteNode
        (*g)->RemoveNode(orig_node);

        VLOG(1) << "MkldnnLayoutRewritePass: New node: " << new_node->DebugString();
        return Status::OK();
    }

    const MkldnnLayoutRewritePass::ContextInfo*
    MkldnnLayoutRewritePass::SearchMatchingContext(const Node* n,
                                                   const Node** fwd_node) {
        CHECK_NOTNULL(n);
        CHECK_NOTNULL(fwd_node);
        *fwd_node = nullptr;

        // Search for matching contextinfo based on node name.
        // There could be more than one matching contextinfos.
        bool is_matching_cinfo_found = false;
        std::vector<const ContextInfo*> mci;
        for (auto ci = cinfo_.cbegin(); ci != cinfo_.cend(); ++ci) {
            if (n->type_string() == (*ci)->node) {
                mci.push_back(*ci);
                is_matching_cinfo_found = true;
            }
        }
        // If no matching contextinfo is found, return immediately.
        if (!is_matching_cinfo_found) {
            return nullptr;
        }

        VLOG(1) << "MkldnnLayoutRewritePass: Searching graph for: " << n->type_string()
                << " in backwards.";

        // Now we will check for forward op name for context info in data
        // flow graph. Get the max hops we should search for the fwd node.
        // We are now going to search (breadth-first) backwards in data
        // dependence graph (for up to max hops) from n for the node
        // specified in fwd.
        // queue to maintain nodes to be visited and depth info for
        // breadth-first search
        std::queue<std::pair<const Node*, int>> nqueue;
        const Node* curr_node = n;
        size_t curr_depth = 0;
        nqueue.push(std::make_pair(curr_node, curr_depth));

        while (curr_depth < kNodeMergeContextMaxDepth && !nqueue.empty()) {
            std::pair<const Node*, int> curr_pair = nqueue.front();
            nqueue.pop();

            std::set<const Node*> visited_nodes;
            curr_node = curr_pair.first;
            curr_depth = curr_pair.second;
            CHECK_NOTNULL(curr_node);

            VLOG(1) << "MkldnnLayoutRewritePass: Visiting node: "
                    << curr_node->type_string() << " at depth: " << curr_depth
                    << " for node: " << n->type_string();

            // If we find a match, we return immediately.
            for (const ContextInfo* ci : mci) {
                if (curr_node->type_string() == ci->fwd) {
                    *fwd_node = curr_node;
                    return ci;
                }
            }

            // Else we explore backward edges from current node.
            // Add the source nodes of all incoming edges of the node to the queue.
            for (const Edge* e : curr_node->in_edges()) {
                // We do not visit already visited node.
                if (visited_nodes.find(e->src()) == visited_nodes.end()) {
                    // Depth of these nodes is 1 more than the depth of current node.
                    nqueue.push(std::make_pair(e->src(), curr_depth + 1));
                    visited_nodes.insert(e->src());
                }
            }
        } /* while */

        return nullptr;
    }

    bool MkldnnLayoutRewritePass::ContextMatchRewrite(const Node* n,
                                                      const ContextInfo* c) {
        const Node* fwd_node = nullptr;
        return SearchMatchingContext(n, &fwd_node) == c;
    }

    const MkldnnLayoutRewritePass::RewriteInfo*
    MkldnnLayoutRewritePass::CheckForNodeRewrite(const Node* n) const {
        CHECK_NOTNULL(n);

        // First check if node along with its type is supported by MKL layer.
        // We do not want to rewrite an op into Mkldnn op if types are not supported.
        // E.g., MkldnnRelu does not support INT32. So we cannot rewrite Relu to
        // MkldnnRelu if type is INT32.
        DataType T;
        if (!GetNodeAttr(n->def(), "T", &T).ok()) {
            return nullptr;
        }

        // BiasAddGrad is not an Mkldnn layer, so we make an exception for it.
        if (n->type_string() != csinfo_.bias_add_grad) {
            if (!mkl_op_registry::IsMkldnnOp(GetMkldnnOpName(n->type_string()), T)) {
                return nullptr;
            }
        }

        // We support 2 types of node rewrites:
        // 1. Rewriting BiasAddGrad depending on its MkldnnConv2DWithBias context.
        // 2. Rewriting an op to Mkldnn op always
        // We return true if any of these 2 conditions is met.

        // Find matching RewriteInfo and then check that rewrite rule applies.
        for (auto ri = rinfo_.cbegin(); ri != rinfo_.cend(); ++ri) {
            if (n->type_string().compare(ri->name) == 0 && ri->rewrite_rule(n, ri->context)) {
                // If we are rewriting BiasAddGrad into BiasAddGrad for MatMul context,
                // then we just return directly.
                if (n->type_string() == csinfo_.bias_add_grad &&
                    ri->context->fwd == csinfo_.matmul &&
                    ri->new_name == csinfo_.bias_add_grad)
                {
                    return nullptr;
                }
                return &*ri;
            }
        }

        // Else return not found.
        return nullptr;
    }

    ///////////////////////////////////////////////////////////////////////////////
    //              Run function for the pass
    ///////////////////////////////////////////////////////////////////////////////

    bool MkldnnLayoutRewritePass::RunPass(std::unique_ptr<Graph>* g) {
        bool result = false;
        CHECK_NOTNULL(g);

        DumpGraph("Before MkldnnLayoutRewritePass", &**g);

        std::vector<Node*> order;
        GetReversePostOrder(**g, &order); // This will give us topological sort.

        for (Node* n : order) {
            // If node is not an op or it cannot run on CPU device, then skip.
            if (!n->IsOp() || !CanOpRunOnCPUDevice(n)) {
                continue;
            }

            const RewriteInfo* ri = nullptr;
            Node* predn = nullptr;
            // We will first search if node is to be rewritten
            if ((ri = CheckForNodeRewrite(n)) != nullptr) {
                string node_name = n->name();
                string op_name = n->type_string();

                VLOG(1) << "MkldnnLayoutRewritePass: Scheduled node " << node_name
                        << " with op " << op_name << " for rewrite using"
                        << " layout optimization.";

                Status s(RewriteNode(g, n, ri));
                if (s.ok()) {
                    VLOG(1) << "MkldnnLayoutRewritePass: rewrote node " << node_name
                            << " with op " << op_name << " for Mkldnn layout optimization.";
                    result = true;
                } else {
                    VLOG(1) << "MkldnnLayoutRewritePass: " << s.error_message();
                }
            } else if ((predn = CheckForNodeMerge(n)) != nullptr) {
                // Otherwise, we will check if the node is to be merged.
                string n1_name = n->name();
                string n2_name = predn->name();

                VLOG(1) << "MkldnnLayoutRewritePass: Scheduled nodes "
                        << n1_name << "=" << n->type_string() << " and "
                        << n2_name << "=" << predn->type_string() << " for merging";

                Status s(MergeNode(g, n, predn));
                if (s.ok()) {
                    VLOG(1) << "MkldnnLayoutRewritePass: Merged nodes " << n1_name << " and "
                            << n2_name;
                    result = true;
                } else {
                    VLOG(1) << "MkldnnLayoutRewritePass: " << s.error_message();
                }
            }
        }

        DumpGraph("After MkldnnLayoutRewritePass", &**g);

        return result;
    }

    bool MkldnnLayoutRewritePass::RunPassUndoWeights(std::unique_ptr<Graph>* g) {
        bool result = false;
        CHECK_NOTNULL(g);

        DumpGraph("Before MkldnnLayoutRewritePass::RunPassUndoWeights", &**g);

        std::vector<Node*> order;
        GetReversePostOrder(**g, &order); // This will give us topological sort.

        for (Node* n : order) {
            // If node is not an op or it cannot run on CPU device, then skip.
            if (!n->IsOp() || !CanOpRunOnCPUDevice(n)) {
                continue;
            }

            if (n->type_string() == csinfo_.conv2d) {
                Status s(InsertUndoWeights(g, n));
                if (s.ok()) {
                    VLOG(1) << "RunPassUndoWeights: Inserted weights conversion for node " << n->name();
                    result = true;
                } else if (s.code() != error::Code::ALREADY_EXISTS) {
                    VLOG(1) << "RunPassUndoWeights: " << s.error_message();
                }
            }
        }

        DumpGraph("After MkldnnLayoutRewritePass::RunPassUndoWeights", &**g);

        return result;
    }

    bool RunMkldnnLayoutRewritePass(std::unique_ptr<Graph>* g) {
        return MkldnnLayoutRewritePass().RunPass(g);
    }

    Status MkldnnLayoutRewritePass::Run(const GraphOptimizationPassOptions& options) {
        if (options.graph == nullptr && options.partition_graphs == nullptr) {
            return Status::OK();
        }

        const bool sessMkldnn = options.session_options
            ->config.arcadia_specific_config_proto().enable_mkldnn_optimization();
        MkldnnUseWeightsAsGiven_ = options.session_options
            ->config.arcadia_specific_config_proto().mkldnn_use_weights_as_given();

        enum class TPassType {
            UNDEF,
            NO_OP,
            REWRITE_TO_MKLDNN,
            UNDO_WEIGHTS
        } passType = TPassType::UNDEF;

        if (sessMkldnn) {
            if (NX86::CachedHaveAVX()) {
                passType = TPassType::REWRITE_TO_MKLDNN;
            } else if (MkldnnUseWeightsAsGiven_) {
                LOG(INFO) << "session requests MKLDNN, but host does not support this";
                passType = TPassType::UNDO_WEIGHTS;
            } else {
                passType = TPassType::NO_OP;
            }
        } else if (MkldnnUseWeightsAsGiven_) {
            passType = TPassType::UNDO_WEIGHTS;
        } else {
            passType = TPassType::NO_OP;
        }

        if (passType == TPassType::REWRITE_TO_MKLDNN) {
            LOG(INFO) << "optimizing graph for MKLDNN";
        }
        if (passType == TPassType::UNDO_WEIGHTS) {
            LOG(INFO) << "session defines weights for MKLDNN, but MKLDNN is not enabled, so undo the weights";
        }
        if (passType == TPassType::NO_OP) {
            return Status::OK();
        }

        auto process_graph = [&](std::unique_ptr<Graph>* g) {
            // Get the ownership of a graph
            std::unique_ptr<Graph>* ng = std::move(g);
            for (bool need_massage = true; need_massage;) {
                if (passType == TPassType::UNDO_WEIGHTS) {
                  need_massage = RunPassUndoWeights(ng);
                } else {
                  need_massage = RunPass(ng);
                }
            }
            // Return the ownership of a graph back
            g->reset(ng->release());
        };

        if (kMkldnnLayoutRewritePassGroup != OptimizationPassRegistry::POST_PARTITIONING) {
            // For any pre-partitioning phase, a graph is stored in options.graph.
            process_graph(options.graph);
        } else {
            // For post partitioning phase, graphs are stored in
            // options.partition_graphs.
            for (auto& pg : *options.partition_graphs) {
                process_graph(&pg.second);
            }
        }

        return Status::OK();
    }

}

#endif /* ARCADIA_BUILD_ROOT */

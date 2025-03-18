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

#include <memory>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "tensorflow/core/common_runtime/function.h"
#include "tensorflow/core/common_runtime/optimization_registry.h"
#include "tensorflow/core/framework/node_def_util.h"
#include "tensorflow/core/graph/algorithm.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/node_builder.h"
#include "tensorflow/core/lib/core/status.h"
#include "tensorflow/core/lib/gtl/map_util.h"
#include "tensorflow/core/lib/hash/hash.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/public/session_options.h"

#include "mkldnn_tfconversion_pass.h"
#include "mkldnn_util.h"

#include <util/system/cpu_id.h>

namespace tensorflow {
    // This pass inserts Mkldnn to Tf tensor conversion nodes (represented by C)
    // in the graph in between A and B, where A and B match any one
    // of the following cases:
    //
    //  1) A = a node that generates output in the Mkldnn format and,
    //     B = a node that does not accept input in the Mkldnn format and,
    //     A -> B (there is a direct edge between A and B, then
    //     We will insert C such that A->C->B.
    //
    //  2) A = a node that generates output in the Mkldnn format and,
    //     B = NULL (in other words, A is the last node in the graph), then
    //     We will insert C such that A->C->B. (C will be the last node.)
    //
    //  Note that case 1 applies to all outputs of A that are input to B.
    //  In other words, the conversions will be required for every output
    //  of A that is input to B. For example, let us say the output of A
    //  is A1, A2, A3, of which A1 and A2 are in Mkldnn format, but A3 is not
    //  in Mkldnn format, and all of them are input to B. In such case, we will
    //  do the conversion for A1 and A2 only. We do not need to do any conversion
    //  for A3.
    //
    // This pass relies on ops registering themselves about their Mkldnn compliance.
    // An Mkldnn-compliant op can accept inputs in the Mkldnn format, and produce outputs
    // in the Mkldnn format. Non-compliant ops accept inputs and outputs in the
    // TensorFlow format.
    //
    class MkldnnToTfConversionPass: public GraphOptimizationPass {
    public:
        MkldnnToTfConversionPass() {
        }
        Status Run(const GraphOptimizationPassOptions& options) override;

        // Insert layout conversion node in the graph pointed by g.
        // Function scans the graph for candidate edges where we
        // need to insert conversion nodes.
        //
        // @return true even if single conversion node is inserted;
        // false, otherwise.
        bool RunPass(std::unique_ptr<Graph>* g);

    private:
        // Is the input Op supported by Mkldnn-specific layout?
        //
        // @input op_name string of the op
        // @input T Datatype to use for checking input op
        // @return true if op is Mkldnn supported; false, otherwise.
        inline bool IsMkldnnSupportedOp(const string& op_name, DataType T) const {
            return mkl_op_registry::IsMkldnnOp(op_name, T);
        }

        // Insert layout conversion node on the edge pointed by 'e' from graph 'g'.
        //
        // Edge will be deleted once a call to this function is successful.
        // Any attempt to use the edge after this call
        // will lead to undefined behaviors.
        //
        // @return Success:OK() if insertion is successful, otherwise returns
        //         appropriate error status code.
        Status InsertConversionNodeOnEdge(std::unique_ptr<Graph>* g, Edge*);
    };

    // We register MkldnnToTf insertion for phase 2 in post-partition grouping
    // because we register MkldnnLayoutRewritePass for phase 1 in post-partition
    // grouping. We register this pass after partitioning so that we get a
    // complete picture of inputs and outputs of the nodes in the graphs.
    const OptimizationPassRegistry::Grouping kMkldnnTfConvPassGroup =
        OptimizationPassRegistry::POST_PARTITIONING;
    REGISTER_OPTIMIZATION(kMkldnnTfConvPassGroup, 2, MkldnnToTfConversionPass);

    Status MkldnnToTfConversionPass::InsertConversionNodeOnEdge(
        std::unique_ptr<Graph>* g, Edge* e) {
        CHECK_NOTNULL(e);

        Node* src = e->src();
        Node* dst = e->dst();

        CHECK_NOTNULL(src);
        CHECK_NOTNULL(dst);

        Node* conversion_node = nullptr;
        DataType src_datatype = DT_INVALID;
        DataType dst_datatype = DT_INVALID;
        string data_format;

        TF_CHECK_OK(GetNodeAttr(src->def(), "T", &src_datatype));
        bool dst_dtype_found = GetNodeAttr(dst->def(), "T", &dst_datatype) ==
                               Status::OK();
        // We compare source and destination datatypes only when both are found.
        if (dst_dtype_found && (src_datatype != dst_datatype)) {
            string err_msg = "T attribute of " + src->name() + " and " +
                             dst->name() + " do not match. Will not insert" +
                             " MkldnnToTf node in such case.";
            return Status(error::Code::INVALID_ARGUMENT, err_msg.c_str());
        }

        // Build the conversion node and specify src as input.
        TF_CHECK_OK(
            NodeBuilder((*g)->NewName("Mkldnn2Tf"), "_MkldnnToTf")
                .Input(src, e->src_output())
                .Input(src, DataIndexToMetaDataIndex(
                                e->src_output(),
                                src->num_outputs())) // Get an Mkldnn tensor slot
                                                     // from the Tf tensor slot.
                .Device(src->def().device())         // We want to get conversion node
                                                     // on same device as source node.
                .Attr("T", src_datatype)
                .Finalize(&**g, &conversion_node));

        CHECK_NOTNULL(conversion_node);
        if (GetNodeAttr(src->def(), "data_format", &data_format) == Status::OK()) {
            conversion_node->AddAttr("data_format", data_format);
        }

        // Get assigned device from source node and apply it to conversion node.
        // We want conversion node to be on the same device as the source node.
        conversion_node->set_assigned_device_name(src->assigned_device_name());

        // Set the Mkldnn op label for this op.
        conversion_node->AddAttr("_kernel", mkl_op_registry::kMkldnnOpLabel);

        // Now that we have added edge from src->conversion_node, let's add edge from
        // output of conversion_node to the dest node. Since conversion_node
        // has only 1 output, the src_output of conversion_node is 0.
        CHECK_NOTNULL((*g)->AddEdge(conversion_node, 0, dst, e->dst_input()));

        VLOG(1) << "MkldnnToTfConversionPass: Inserted " << conversion_node->name() << " on: "
                << src->type_string() << ":" << src->name() << " and "
                << dst->type_string() << ":" << dst->name();

        // Remove src->dst edge now.
        (*g)->RemoveEdge(e);
        return Status::OK();
    }

    bool MkldnnToTfConversionPass::RunPass(std::unique_ptr<Graph>* g) {
        bool result = false;

        CHECK_NOTNULL(g);

        DumpGraph("Before MkldnnToTfConversionPass", &**g);

        // Since we are looking for an Mkldnn-supported op node immediately
        // followed by a non-Mkldnn op node, we will just iterate over edge
        // set of the graph.
        // edge set whose source and destination are candidates for
        // inserting conversion node
        std::vector<Edge*> candidate_edges;

        for (const Edge* e : (*g)->edges()) {
            Node* src = e->src();
            Node* dst = e->dst();

            // We skip control edges.
            if (e->IsControlEdge()) {
                continue;
            }

            // We skip adding MkldnnToTf on an edge between X->MkldnnToTf or
            // MkldnnToTf->X, where X is any node.
            if (src->type_string().compare("_MkldnnToTf") == 0 ||
                dst->type_string().compare("_MkldnnToTf") == 0) {
                continue;
            }

            VLOG(1) << "MkldnnToTfConversionPass: InsertConversionNodes: "
                    << src->type_string() << " and " << dst->type_string();

            // Let's get source and destination data type.
            // We cannot check datatype on destination node because destination node
            // may not be Mkldnn node.
            DataType src_datatype;
            DataType dst_datatype;
            bool src_is_mkl_op = (GetNodeAttr(src->def(), "T", &src_datatype) ==
                                      Status::OK() &&
                                  IsMkldnnSupportedOp(src->type_string(), src_datatype));
            bool dst_is_mkl_op = (GetNodeAttr(dst->def(), "T", &dst_datatype) ==
                                      Status::OK() &&
                                  IsMkldnnSupportedOp(dst->type_string(), dst_datatype));

            // Check if src with is Mkldnn-compliant, while dst is not Mkldnn-compliant.
            if (src_is_mkl_op && !dst_is_mkl_op) {
                VLOG(1) << "MkldnnToTfConversionPass: Scheduled nodes " << src->name()
                        << " and " << dst->name() << " for inserting conversion nodes";
                candidate_edges.push_back(const_cast<Edge*>(e));
            }
        }

        // Process all candidate edges and insert conversion nodes on them.
        for (Edge* e : candidate_edges) {
            // Even if we insert conversion node on a single edge, we
            // need to return true.
            string src_name = e->src()->name();
            string dst_name = e->dst()->name();
            if (InsertConversionNodeOnEdge(g, e) == Status::OK()) {
                VLOG(1) << "MkldnnToTfConversionPass: Inserted conversion "
                        << "node on edge between " << src_name << " and " << dst_name;
                result = true;
            }
        }

        DumpGraph("After MkldnnToTfConversionPass", &**g);

        // We need to return true even if we insert one conversion node
        // anywhere in the graph.
        return result;
    }

    //////////////////////////////////////////////////////////////////////////////
    //              Run function for the pass
    //////////////////////////////////////////////////////////////////////////////

    bool InsertMkldnnToTfConversionNodes(std::unique_ptr<Graph>* g) {
        return MkldnnToTfConversionPass().RunPass(g);
    }

    Status MkldnnToTfConversionPass::Run(const GraphOptimizationPassOptions& options) {
        if (options.graph == nullptr && options.partition_graphs == nullptr) {
            return Status::OK();
        }
        bool mkldnnEnabled = options.session_options
            ->config.arcadia_specific_config_proto().enable_mkldnn_optimization();
        if (!(mkldnnEnabled && NX86::CachedHaveAVX())) {
            return Status::OK();
        }

        auto process_graph = [&](std::unique_ptr<Graph>* g) {
            // Get the ownership of graph
            std::unique_ptr<Graph>* ng = std::move(g);
            RunPass(ng);
            // Return the ownership of graph back
            g->reset(ng->release());
        };

        if (kMkldnnTfConvPassGroup != OptimizationPassRegistry::POST_PARTITIONING) {
            // For any pre-partitioning phase, graph is stored in options.graph.
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

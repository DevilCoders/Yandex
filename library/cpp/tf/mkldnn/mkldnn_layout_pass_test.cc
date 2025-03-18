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

#include "mkldnn_layout_pass.h"
#include "mkldnn_util.h"

#include <algorithm>
#include <string>
#include <vector>

#include "tensorflow/core/framework/op.h"
#include "tensorflow/core/framework/tensor.h"
#include "tensorflow/core/graph/graph.h"
#include "tensorflow/core/graph/graph_constructor.h"
#include "tensorflow/core/graph/testlib.h"
#include "tensorflow/core/kernels/ops_util.h"
#include "tensorflow/core/lib/random/simple_philox.h"
#include "tensorflow/core/lib/strings/str_util.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/protobuf.h"
#include "tensorflow/core/platform/test.h"
#include "tensorflow/core/platform/test_benchmark.h"

namespace tensorflow {
    namespace {
        const char kCPUDevice[] = "/job:a/replica:0/task:0/cpu:0";
        const char kGPUDevice[] = "/job:a/replica:0/task:0/gpu:0";

        void InitGraph(const string& s, Graph* graph,
                              const string& device = kCPUDevice) {
            GraphDef graph_def;

            auto parser = protobuf::TextFormat::Parser();
            //  parser.AllowRelaxedWhitespace(true);
            CHECK(parser.MergeFromString(s, &graph_def)) << s;
            GraphConstructorOptions opts;
            TF_CHECK_OK(ConvertGraphDefToGraph(opts, graph_def, graph));

            for (Node* node : graph->nodes()) {
                node->set_assigned_device_name(device);
            }
        }

        class MkldnnLayoutPassTest: public ::testing::Test {
        public:
            MkldnnLayoutPassTest()
                : graph_(OpRegistry::Global())
            {
            }

            void InitGraph(const string& s, const string& device = kCPUDevice) {
                ::tensorflow::InitGraph(s, &graph_, device);
                original_ = CanonicalGraphString(&graph_);
            }

            static bool IncludeNode(const Node* n) {
                return n->IsOp();
            }

            static string EdgeId(const Node* n, int index) {
                if (index == 0) {
                    return n->name();
                } else if (index == Graph::kControlSlot) {
                    return strings::StrCat(n->name(), ":control");
                } else {
                    return strings::StrCat(n->name(), ":", index);
                }
            }

            string CanonicalGraphString(Graph* g) {
                std::vector<string> nodes;
                std::vector<string> edges;
                for (const Node* n : g->nodes()) {
                    if (IncludeNode(n)) {
                        nodes.push_back(strings::StrCat(n->name(), "(", n->type_string(), ")"));
                    }
                }
                for (const Edge* e : g->edges()) {
                    if (IncludeNode(e->src()) && IncludeNode(e->dst())) {
                        edges.push_back(strings::StrCat(EdgeId(e->src(), e->src_output()), "->",
                                                        EdgeId(e->dst(), e->dst_input())));
                    }
                }
                // Canonicalize
                std::sort(nodes.begin(), nodes.end());
                std::sort(edges.begin(), edges.end());
                return strings::StrCat(str_util::Join(nodes, ";"), "|",
                                       str_util::Join(edges, ";"));
            }

            string DoMkldnnLayoutOptimizationPass() {
                string before = CanonicalGraphString(&graph_);
                LOG(ERROR) << "Before MKLDNN layout rewrite pass: " << before;

                std::unique_ptr<Graph> ug(&graph_);
                RunMkldnnLayoutRewritePass(&ug);
                ug.release();

                string result = CanonicalGraphString(&graph_);
                LOG(ERROR) << "After MKLDNN layout rewrite pass:  " << result;
                return result;
            }

            const string& OriginalGraph() const {
                return original_;
            }

            Graph graph_;
            string original_;
        };

        REGISTER_OP("Input").Output("o: float").SetIsStateful();
        REGISTER_OP("InputList").Output("o: N * float").Attr("N: int").SetIsStateful();
        REGISTER_OP("InputWithDataFormat").Output("o: float").Attr("data_format: string").SetIsStateful();
        REGISTER_OP("HalfInput").Output("o: half").SetIsStateful();
        REGISTER_OP("Int32Input").Output("o: int32").SetIsStateful();
        REGISTER_OP("_MklInput").Output("o: uint8").SetIsStateful();
        REGISTER_OP("_MklInput2").Output("o: uint8").Output("o1: uint8").SetIsStateful();

        /////////////////////////////////////////////////////////////////////
        //  Unit tests related to node merge optiimization
        /////////////////////////////////////////////////////////////////////

        TEST_F(MkldnnLayoutPassTest, Basic) {
            InitGraph(
                "node { name: 'A' op: 'Input'}"
                "node { name: 'B' op: 'Input'}"
                "node { name: 'C' op: 'Mul' attr { key: 'T' value { type: DT_FLOAT } }"
                " input: ['A', 'B'] }"
                "node { name: 'D' op: 'Mul' attr { key: 'T' value { type: DT_FLOAT } }"
                " input: ['A', 'B'] }");
            EXPECT_EQ(DoMkldnnLayoutOptimizationPass(),
                      "A(Input);B(Input);C(Mul);D(Mul)|"
                      "A->C;A->D;B->C:1;B->D:1");
        }

        TEST_F(MkldnnLayoutPassTest, Conv2D) {
            ASSERT_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
            InitGraph(
                "node { name: 'A' op: 'Input'}"
                "node { name: 'B' op: 'Input'}"
                "node { name: 'C' op: 'Conv2D'"
                "  attr { key: 'T'                value { type: DT_FLOAT } }"
                "  attr { key: 'data_format'      value { s: 'NCHW' } }"
                "  attr { key: 'use_cudnn_on_gpu' value { b: false } }"
                "  attr { key: 'strides'          value { list: {i:1, i:1, i:1, i:1} } }"
                "  attr { key: 'padding'          value { s: 'SAME' } }"
                "  input: ['A', 'B']}");
            EXPECT_STREQ(DoMkldnnLayoutOptimizationPass().c_str(),
                         "A(Input);B(Input);C(_MkldnnConv2D);DMT/_0(Const);DMT/_1(Const)|"
                         "A->C;A:control->DMT/_0:control;A:control->DMT/_1:control;"
                         "B->C:1;DMT/_0->C:2;DMT/_1->C:3");
        }

        TEST_F(MkldnnLayoutPassTest, Concat0) {
            ASSERT_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
            InitGraph(R"(
                node { name: 'A1' op: 'Input' }
                node { name: 'A2' op: 'Input' }
                node { name: 'Aconcat_dim' op: 'Int32Input' }
                node { name: 'C' op: 'Concat'
                    attr { key: 'T'      value { type: DT_FLOAT } }
                    attr { key: 'N'      value { i: 2 } }
                    input: ['Aconcat_dim', 'A1', 'A2']
                    }
                )");
            EXPECT_STREQ(DoMkldnnLayoutOptimizationPass().c_str(),
                         "A1(Input);A2(Input);Aconcat_dim(Int32Input);C(Concat)|"
                         "A1->C:1;A2->C:2;Aconcat_dim->C");
        }

        TEST_F(MkldnnLayoutPassTest, Concat1) {
            ASSERT_EQ(kTensorOrdering, MkldnnTfTensorOrdering::TENSORS_CONTIGUOUS);
            InitGraph(R"(
                node { name: 'A1' op: 'InputWithDataFormat' attr { key: 'data_format' value {s: 'NCHW'}}}
                node { name: 'A2' op: 'InputWithDataFormat' attr { key: 'data_format' value {s: 'NCHW'}}}
                node { name: 'Aconcat_dim' op: 'Int32Input' }
                node { name: 'C' op: 'Concat'
                    attr { key: 'T'      value { type: DT_FLOAT } }
                    attr { key: 'N'      value { i: 2 } }
                    input: ['Aconcat_dim', 'A1', 'A2']
                    }
                )");
            EXPECT_STREQ(DoMkldnnLayoutOptimizationPass().c_str(),
                "A1(InputWithDataFormat);A2(InputWithDataFormat);Aconcat_dim(Int32Input);C(_MkldnnConcat);DMT/_0(Const);DMT/_1(Const);DMT/_2(Const)|"
                "A1->C:1;A2->C:2;Aconcat_dim->C;"
                "Aconcat_dim:control->DMT/_0:control;Aconcat_dim:control->DMT/_1:control;Aconcat_dim:control->DMT/_2:control;"
                "DMT/_0->C:3;DMT/_1->C:4;DMT/_2->C:5");
        }

        /////////////////////////////////////////////////////////////////////

        void BM_MkldnnLayoutRewritePass(int iters, int op_nodes) {
            testing::StopTiming();
            string s;
            for (int in = 0; in < 10; in++) {
                s += strings::Printf("node { name: 'in%04d' op: 'Input'}", in);
            }
            random::PhiloxRandom philox(301, 17);
            random::SimplePhilox rnd(&philox);
            for (int op = 0; op < op_nodes; op++) {
                s += strings::Printf(
                    "node { name: 'op%04d' op: 'Mul' attr { key: 'T' value { "
                    "type: DT_FLOAT } } input: ['in%04d', 'in%04d' ] }",
                    op, rnd.Uniform(10), rnd.Uniform(10));
            }

            bool first = true;
            while (iters > 0) {
                Graph* graph = new Graph(OpRegistry::Global());
                InitGraph(s, graph);
                int N = graph->num_node_ids();
                if (first) {
                    testing::SetLabel(strings::StrCat("Per graph node.  Nodes: ", N));
                    first = false;
                }
                {
                    testing::StartTiming();
                    std::unique_ptr<Graph> ug(graph);
                    RunMkldnnLayoutRewritePass(&ug);
                    testing::StopTiming();
                }
                iters -= N; // Our benchmark units are individual graph nodes,
                            // not whole graphs
                            // delete graph;
            }
        }
        BENCHMARK(BM_MkldnnLayoutRewritePass)->Arg(1000)->Arg(10000);

    }
}

#endif /* ARCADIA_BUILD_ROOT */

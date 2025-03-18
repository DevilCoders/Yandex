#include "graph_builder.h"

#include <library/cpp/trace_usage/protos/event.pb.h>

#include <util/stream/file.h>

namespace NTraceUsage {
    void TGraphBuilder::TNode::TEdge::AddTime(TDuration duration) {
        TotalDuration += duration;
        ++NumExecutions;
    }
    void TGraphBuilder::TNode::TEdge::FlushEdge(size_t sourceNodeId, size_t targetNodeId, TDuration maxDuration, IOutputStream& output) const {
        size_t fontSize = 8 + TotalDuration.SecondsFloat() * 10 / Max<double>(maxDuration.SecondsFloat(), 0.001);
        output << "        node_" << sourceNodeId << " -> node_" << targetNodeId << "[label=\"" << NumExecutions << "\", fontsize=" << fontSize << "]" << Endl;
    }

    void TGraphBuilder::TNode::AddTime(TDuration duration) {
        TotalDuration += duration;
        ++NumExecutions;
    }
    void TGraphBuilder::TNode::AddEdge(size_t subNodeId, TDuration duration) {
        SubNodes[subNodeId].AddTime(duration);
    }
    void TGraphBuilder::TNode::AddEdge(size_t subNodeId) {
        SubNodes[subNodeId];
    }
    void TGraphBuilder::TNode::FlushNode(const TNodeKey& node, TDuration maxDuration, IOutputStream& output) const {
        TDuration knownSubDurations;
        for (const auto& nodeId2EdgePair : SubNodes) {
            knownSubDurations += nodeId2EdgePair.second.TotalDuration;
        }
        double diff = TotalDuration.SecondsFloat() - knownSubDurations.SecondsFloat();
        size_t fontSize = 8 + Max(diff, 0.) * 10 / Max<double>(maxDuration.SecondsFloat(), 0.001);
        output << "        node_" << NodeId << " [label=\"" << node.GetFunction() << "\\n"
               << NumExecutions << "\\n"
               << diff / Max<double>(NumExecutions, 1.) << "\\n"
               << TotalDuration.SecondsFloat() / Max<double>(NumExecutions, 1.) << "\\n"
               << diff << "\\n"
               << TotalDuration.SecondsFloat() << "\", fontsize=" << fontSize << "]" << Endl;
    }
    void TGraphBuilder::TNode::FlushEdges(TDuration maxDuration, IOutputStream& output) const {
        for (const auto& nodeId2EdgePair : SubNodes) {
            size_t targetNodeId = nodeId2EdgePair.first;
            nodeId2EdgePair.second.FlushEdge(NodeId, targetNodeId, maxDuration, output);
        }
    }

    TGraphBuilder::TNode& TGraphBuilder::TThreadSubgraph::GetNode(const TNodeKey& function, size_t& lastNodeId) {
        TNode& ret = Name2Node[function];
        if (!ret.NodeId) {
            ret.NodeId = ++lastNodeId;
        }
        return ret;
    }
    void TGraphBuilder::TThreadSubgraph::AddTime(const TNodeKey& function, size_t& lastNodeId, TDuration duration) {
        GetNode(function, lastNodeId).AddTime(duration);
    }
    void TGraphBuilder::TThreadSubgraph::AddEdge(const TNodeKey& supFunction, const TNodeKey& subFunction, size_t& lastNodeId, TDuration duration) {
        TNode& supNode = GetNode(supFunction, lastNodeId);
        TNode& subNode = GetNode(subFunction, lastNodeId);
        supNode.AddEdge(subNode.NodeId, duration);
    }
    void TGraphBuilder::TThreadSubgraph::AddEdge(const TNodeKey& supFunction, const TNodeKey& subFunction, size_t& lastNodeId) {
        TNode& supNode = GetNode(supFunction, lastNodeId);
        TNode& subNode = GetNode(subFunction, lastNodeId);
        supNode.AddEdge(subNode.NodeId);
    }
    void TGraphBuilder::TThreadSubgraph::FlushNodes(TDuration maxDuration, IOutputStream& output) const {
        for (const auto& name2NodePair : Name2Node) {
            const TNodeKey& name = name2NodePair.first;
            name2NodePair.second.FlushNode(name, maxDuration, output);
        }
    }
    void TGraphBuilder::TThreadSubgraph::FlushEdges(TDuration maxDuration, IOutputStream& output) const {
        for (const auto& name2NodePair : Name2Node) {
            name2NodePair.second.FlushEdges(maxDuration, output);
        }
    }

    void TGraphBuilder::TThreadSubgraph::StartFunction(const TString& function, TInstant start, EMetaType meta, size_t& lastNodeId) {
        TNodeKey nodeKey(Stack.size(), function, meta);
        GetNode(nodeKey, lastNodeId);
        if (Stack.size()) {
            const auto& back = Stack.back();
            TNodeKey supNodeKey(Stack.size() - 1, back.Function, back.Meta);
            AddEdge(supNodeKey, nodeKey, lastNodeId);
        }
        Stack.emplace_back();
        auto& scope = Stack.back();
        scope.Function = function;
        scope.Meta = meta;
        scope.Start = start;
    }
    void TGraphBuilder::TThreadSubgraph::FinishFunction(const TString& function, TInstant finish, EMetaType meta, size_t& lastNodeId) {
        if (Stack.empty()) {
            return; // TODO: smart hacks
        }
        if (Stack.back().Function != function) {
            size_t j = Stack.size();
            while (true) {
                if (j == 0) {
                    Stack.clear();
                    break;
                }
                if (Stack[--j].Function == function) { // Some trash, let's remove
                    Stack.resize(j + 1);
                    break;
                }
            }
        }
        if (Stack.empty() || Stack.back().Function != function) {
            return;
        }
        TInstant start = Stack.back().Start;
        TDuration duration = finish - start;
        TNodeKey nodeKey(Stack.size() - 1, function, meta);
        AddTime(nodeKey, lastNodeId, duration);
        Stack.pop_back();
        if (!Stack.empty()) {
            const auto& back = Stack.back();
            TNodeKey supNodeKey(Stack.size() - 1, back.Function, back.Meta);
            AddEdge(supNodeKey, nodeKey, lastNodeId, duration);
        }
    }
    void TGraphBuilder::TThreadSubgraph::Flush(TDuration maxDuration, IOutputStream& output) const {
        FlushNodes(maxDuration, output);
        FlushEdges(maxDuration, output);
    }
    TDuration TGraphBuilder::TThreadSubgraph::GetMaxDuration() const {
        TDuration maxDuration;
        for (const auto& name2NodePair : Name2Node) {
            TDuration nodeDuration = name2NodePair.second.TotalDuration;
            if (maxDuration < nodeDuration) {
                maxDuration = nodeDuration;
            }
        }
        return maxDuration;
    }

    TDuration TGraphBuilder::GetMaxDuration() const {
        TDuration maxDuration;
        for (const auto& threadInfo : ThreadId2Subgraph) {
            TDuration threadMaxDuration = threadInfo.second.GetMaxDuration();
            if (maxDuration < threadMaxDuration) {
                maxDuration = threadMaxDuration;
            }
        }
        return maxDuration;
    }

    void TGraphBuilder::ProcessEvent(const TEventReportProto& eventReport) {
        const auto& info = eventReport.GetCommonEventData();
        TInstant eventTime = TInstant::MicroSeconds(info.GetMicroSecondsTime());
        size_t threadId = info.GetThreadId();
        for (size_t i = 0; i < eventReport.StartFunctionScopeSize(); ++i) {
            const auto& scope = eventReport.GetStartFunctionScope(i);
            TString function = scope.GetFunction();
            ThreadId2Subgraph[threadId].StartFunction(function, /*start=*/eventTime, /*meta=*/MT_FUNCTION, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.CloseFunctionScopeSize(); ++i) {
            const auto& scope = eventReport.GetCloseFunctionScope(i);
            TString function = scope.GetFunction();
            ThreadId2Subgraph[threadId].FinishFunction(function, /*finish=*/eventTime, /*meta=*/MT_FUNCTION, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.StartAcquiringMutexSize(); ++i) {
            ThreadId2Subgraph[threadId].StartFunction("mutex(ext)", /*start=*/eventTime, /*meta=*/MT_MUTEX_EXT, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.AcquiredMutexSize(); ++i) {
            ThreadId2Subgraph[threadId].StartFunction("mutex(int)", /*start=*/eventTime, /*meta=*/MT_MUTEX_INT, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.StartReleasingMutexSize(); ++i) {
            ThreadId2Subgraph[threadId].FinishFunction("mutex(int)", /*start=*/eventTime, /*meta=*/MT_MUTEX_INT, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.ReleasedMutexSize(); ++i) {
            ThreadId2Subgraph[threadId].FinishFunction("mutex(ext)", /*start=*/eventTime, /*meta=*/MT_MUTEX_EXT, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.StartWaitEventSize(); ++i) {
            ThreadId2Subgraph[threadId].StartFunction("wait_event", /*start=*/eventTime, /*meta=*/MT_MUTEX_INT, LastNodeId);
        }
        for (size_t i = 0; i < eventReport.FinishWaitEventSize(); ++i) {
            ThreadId2Subgraph[threadId].FinishFunction("wait_event", /*start=*/eventTime, /*meta=*/MT_MUTEX_INT, LastNodeId);
        }
    }

    void TGraphBuilder::Flush(const TString& fileName) {
        TOFStream dotFile(fileName);
        dotFile << "digraph \"callgraph\" {" << Endl;
        dotFile << "    node [shape=rect]" << Endl;
        TDuration maxDuration = GetMaxDuration();
        for (auto& threadInfo : ThreadId2Subgraph) {
            size_t threadId = threadInfo.first;
            dotFile << "    subgraph cluster_" << threadId << " {" << Endl;
            dotFile << "        label=\"" << threadId << "\"" << Endl;
            threadInfo.second.Flush(maxDuration, dotFile);
            dotFile << "    }" << Endl;
        }
        dotFile << "}" << Endl;
    }

}

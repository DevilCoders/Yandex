#pragma once

#include "event_processor.h"

#include <util/datetime/base.h>
#include <util/generic/map.h>
#include <util/generic/vector.h>

namespace NTraceUsage {
    class TGraphBuilder: public IEventProcessor {
    private:
        enum EMetaType {
            MT_FUNCTION,
            MT_MUTEX_EXT,
            MT_MUTEX_INT,
            MT_WAIT_EVENT
        };
        class TNodeKey {
        private:
            size_t StackSize = 0; // Above
            TString Function;
            EMetaType Meta = MT_FUNCTION;

        public:
            TNodeKey(size_t stackSize, const TString& function, EMetaType meta)
                : StackSize(stackSize)
                , Function(function)
                , Meta(meta)
            {
            }

            const TString& GetFunction() const {
                return Function;
            }

            bool operator<(const TNodeKey& other) const {
                if (Function != other.Function) {
                    return Function < other.Function;
                }
                if (Meta != other.Meta) {
                    return Meta < other.Meta;
                }
                return StackSize < other.StackSize;
            }
        };
        struct TNode {
            struct TEdge {
                TDuration TotalDuration;
                size_t NumExecutions = 0;

                void AddTime(TDuration duration);
                void FlushEdge(size_t sourceNodeId, size_t targetNodeId, TDuration maxDuration, IOutputStream& output) const;
            };

            TDuration TotalDuration;
            size_t NumExecutions = 0;
            size_t NodeId = 0;
            TMap<size_t, TEdge> SubNodes;

            void AddTime(TDuration duration);

            void AddEdge(size_t subNodeId, TDuration duration);
            void AddEdge(size_t subNodeId);

            void FlushNode(const TNodeKey& node, TDuration maxDuration, IOutputStream& output) const;
            void FlushEdges(TDuration maxDuration, IOutputStream& output) const;
        };
        using TName2Node = TMap<TNodeKey, TNode>;

        struct TOpenedScope {
            TString Function;
            EMetaType Meta = MT_FUNCTION;
            TInstant Start;
        };
        using TOpenedStack = TVector<TOpenedScope>;

        class TThreadSubgraph {
        private:
            TName2Node Name2Node;
            TOpenedStack Stack;

            TNode& GetNode(const TNodeKey& function, size_t& lastNodeId);
            void AddTime(const TNodeKey& function, size_t& lastNodeId, TDuration duration);
            void AddEdge(const TNodeKey& supFunction, const TNodeKey& subFunction, size_t& lastNodeId, TDuration duration);
            void AddEdge(const TNodeKey& supFunction, const TNodeKey& subFunction, size_t& lastNodeId);
            void FlushNodes(TDuration maxDuration, IOutputStream& output) const;
            void FlushEdges(TDuration maxDuration, IOutputStream& output) const;

        public:
            void StartFunction(const TString& function, TInstant start, EMetaType meta, size_t& lastNodeId);
            void FinishFunction(const TString& function, TInstant finish, EMetaType meta, size_t& lastNodeId);

            void Flush(TDuration maxDuration, IOutputStream& output) const;
            TDuration GetMaxDuration() const;
        };
        using TThreadId2Subgraph = TMap<size_t, TThreadSubgraph>;

        TThreadId2Subgraph ThreadId2Subgraph;
        size_t LastNodeId;

        TDuration GetMaxDuration() const;

    public:
        void ProcessEvent(const TEventReportProto& eventReport) final;
        void Flush(const TString& fileName);
    };

}

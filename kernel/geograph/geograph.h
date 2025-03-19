#pragma once

#include <kernel/gazetteer/articlepool.h>
#include <kernel/gazetteer/gazetteer.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/memory/blob.h>
#include <util/memory/pool.h>
#include <util/stream/output.h>
#include <util/system/unaligned_mem.h>

namespace NGeoGraph {
    class TGeoGraph {
    private:
        struct TNode {
            TWtringBuf Title;
            size_t ParentBegin;
            size_t ParentEnd;
        };

    public:
        TGeoGraph();
        TGeoGraph(const NGzt::TGazetteer* gazetteer) : TGeoGraph() {
            Init(gazetteer);
        }

        TGeoGraph(TBlob source) : TGeoGraph() {
            Init(source);
        }

        void Init(const NGzt::TGazetteer* gazetteer);
        void Init(TBlob);
        void Save(IOutputStream&) const;

        // обход родителей из точки заданной title
        // traverseAll=true -  обходить всех (кроме родителей func() == true)
        // traverseAll=false - обходить до первого func() == true
        template <class TFunctor>
        bool TraverseGeoParts(const TWtringBuf title, TFunctor&& func, bool traverseAll) const {
            auto nodeId = Index.find(title.Before('/'));
            if (nodeId == Index.end())
                return false;

            TVector<const TNode*> stack = {&Nodes[nodeId->second]};
            while (stack) {
                const TNode* node = stack.back();
                stack.pop_back();
                for (size_t parentIdx = node->ParentBegin; parentIdx != node->ParentEnd; ++parentIdx) {
                    const TNode* parent = &Nodes[ReadUnaligned<size_t>(ParentsPtr + parentIdx)];
                    if (func(parent->Title)) {
                        if (traverseAll)
                            continue;
                        else
                            return true;
                    }
                    stack.push_back(parent);
                }
            }
            return false;
        }

    private:
        TBlob Source;
        TMemoryPool MemoryPool;
        TVector<TNode> Nodes;
        TVector<size_t> Parents; // [Nodes[i].ParentBegin, Nodes[i].ParentEnd) are the parents of node `i`
        const size_t* ParentsPtr; // may point to blob contents or to Parents.data()
        THashMap<TWtringBuf, size_t, THash<TWtringBuf>, TEqualTo<TWtringBuf>, TPoolAllocator> Index; // title -> node index
    };

    template <class TFunctor>
    bool TraverseGeoPartsAll(const NGzt::TArticlePtr& article, TFunctor& func) {
        if (const TGeoGraph* graph = article.GetArticlePool()->GetGeoGraph<TGeoGraph>())
            return graph->TraverseGeoParts(article.GetTitle(), std::forward<TFunctor>(func), true);
        return false;
    }

    template <class TFunctor>
    bool TraverseGeoPartsFirst(const NGzt::TArticlePtr& article, TFunctor& func) {
        if (const TGeoGraph* graph = article.GetArticlePool()->GetGeoGraph<TGeoGraph>())
            return graph->TraverseGeoParts(article.GetTitle(), std::forward<TFunctor>(func), false);
        return false;
    }
}

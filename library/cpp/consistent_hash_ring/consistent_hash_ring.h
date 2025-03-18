#pragma once

#include "point_generator.h"

#include <util/generic/hash_set.h>
#include <util/generic/map.h>

template <typename TNode, typename TPoint = ui64>
class TConsistentHashRing {
    static constexpr ui64 DEF_VIRTUAL_NODE_COUNT = 200;

    struct TNodeData {
        bool Disabled = false;
    };

    using TNodeStorage = THashMap<TNode, TNodeData>;
    using TLookupMap = TMap<TPoint, typename TNodeStorage::const_iterator>;

public:
    TConsistentHashRing()
        : TConsistentHashRing(TRandomPointGenerator<TNode>())
    {
    }

    template <typename TPointGenerator>
    TConsistentHashRing(TPointGenerator pointGenerator)
        : PointGenerator(new TPointGeneratorModel<TNode, TPoint, TPointGenerator>(std::move(pointGenerator)))
    {
    }

    void AddNode(const TNode& node, ui64 virtualNodeCount = DEF_VIRTUAL_NODE_COUNT) {
        auto inserted = NodeStorage.insert(std::make_pair(node, TNodeData()));

        if (!inserted.second) {
            return;
        }

        PointGenerator->NextNode(node);

        for (ui64 i = 0; i < virtualNodeCount; ++i) {
            auto point = PointGenerator->NextPoint();
            LookupMap.insert(std::make_pair(point, inserted.first));
        }
    }

    void DisableNode(const TNode& node) {
        TNodeData& nodeData = FindNodeData(node);
        nodeData.Disabled = true;
    }

    void EnableNode(const TNode& node) {
        TNodeData& nodeData = FindNodeData(node);
        nodeData.Disabled = false;
    }

    TNode FindNode(TPoint point) const {
        if (NodeStorage.empty()) {
            return TNode();
        }

        auto it = LookupMap.lower_bound(point);

        if (it == LookupMap.end()) {
            it = LookupMap.begin();
        }

        auto startedFrom = it;

        do {
            if (!it->second->second.Disabled) {
                return it->second->first;
            }

            if (++it == LookupMap.end()) {
                it = LookupMap.begin();
            }
        } while (it != startedFrom);

        return TNode();
    }

private:
    TNodeData& FindNodeData(const TNode& node) {
        auto it = NodeStorage.find(node);
        if (it == NodeStorage.end()) {
            ythrow yexception() << "Node not found";
        }

        return it->second;
    }

private:
    TNodeStorage NodeStorage;
    TLookupMap LookupMap;
    TSimpleSharedPtr<TPointGeneratorBase<TNode, TPoint>> PointGenerator;
};

#pragma once

#include "utility.h"

#include <util/generic/fwd.h>
#include <util/memory/pool.h>

namespace NOffroad {
    namespace NPrivate {
        template <class Vec>
        struct TDefaultVecTraits {
            static bool ValidPrefix(const Vec& /*vec*/) {
                return true;
            }
        };

        template <class Vec, size_t vecSize, size_t vecBits, class VecTraits = TDefaultVecTraits<Vec>>
        class TGreedyPrefixGroupBuilder {
        public:
            using TVec = Vec;

            enum {
                VecSize = vecSize,
                VecBits = vecBits,
                LevelSize = (ui32(1) << vecSize)
            };

            TGreedyPrefixGroupBuilder()
                : Pool_(1000 * sizeof(TPrefixTreeNode<LevelSize>), TMemoryPool::TLinearGrow::Instance())
                , NodesCount_(1)
                , Root_(Pool_.New<TPrefixTreeNode<LevelSize>>(static_cast<ui8>(0)))
            {
                Add(TVec(ui32(0)), 0);
            }

            void Reset() {
                Pool_.Clear();
                NodesCount_ = 1;
                Root_ = Pool_.New<TPrefixTreeNode<LevelSize>>(static_cast<ui8>(0));
                Stage_ = Sampling;
                Add(TVec(ui32(0)), 0);
            }

            bool IsFinished() const {
                return Stage_ == Finished;
            }

            void Add(const TVec& data, size_t count = 1) {
                Y_VERIFY(Stage_ == Sampling);

                TPrefixTreeNode<LevelSize>* current = Root_;
                TVec prefix = TVec(ui32(0));
                for (size_t i = 0; i < VecBits; ++i) {
                    ui32 level = GetLevel(data, i);
                    prefix = NextPrefix(prefix, level);
                    if (!VecTraits::ValidPrefix(prefix)) {
                        break;
                    }
                    if (!(current->NextNode[level])) {
                        ++NodesCount_;
                        current->NextNode[level] = Pool_.New<TPrefixTreeNode<LevelSize>>(static_cast<ui8>(current->Level + 1), current);
                    }
                    current = current->NextNode[level];
                }
                current->Weight += count;
            }

            template <class PrefixGroup, class WeightedPrefixGroupSet>
            void BuildGroups(ui32 bits, ui32 groupsCount, WeightedPrefixGroupSet* groups) {
                Y_VERIFY(Stage_ != Finished);
                Stage_ = Finished;

                Y_ENSURE(groupsCount > 0);
                Y_ENSURE(bits + 1 <= groupsCount);
                Y_ENSURE(bits <= VecBits);

                groups->clear();

                TWeightOptimizationRangeMinimumTree<LevelSize> rmq(NodesCount_, Root_);

                TVector<TPrefixTreeNode<LevelSize>*> selectedNodes;
                selectedNodes.reserve(groupsCount);
                selectedNodes.push_back(Root_);

                TVector<TPrefixTreeNode<LevelSize>*> zeroGroupNodes;
                zeroGroupNodes.reserve(bits + 1);

                TPrefixTreeNode<LevelSize>* current = Root_;
                for (size_t i = 0; i <= VecBits; ++i) {
                    if (VecBits - i <= bits) {
                        (*groups)[PrefixGroup(TVec(ui32(0)), VecBits - i)] = Max<ui64>() / 2;
                        zeroGroupNodes.push_back(current);
                        if (i > 0) {
                            rmq.IncludeInCover(current);
                            selectedNodes.push_back(current);
                        }
                    }
                    if (i < VecBits) {
                        Y_ENSURE(current->NextNode[0]);
                        current = current->NextNode[0];
                    }
                }

                TVector<TPrefixTreeNode<LevelSize>*> nextSelectedNodes;
                nextSelectedNodes.reserve(groupsCount);

                for (;;) {
                    while (selectedNodes.size() < groupsCount) {
                        TPrefixTreeNode<LevelSize>* v = rmq.MaxDeltaNode();
                        if (!v) {
                            break;
                        }
                        selectedNodes.push_back(v);
                        rmq.IncludeInCover(v);
                    }
                    nextSelectedNodes.clear();
                    size_t zeroGroupNodesPtr = 0;
                    for (TPrefixTreeNode<LevelSize>* v : selectedNodes) {
                        if (zeroGroupNodesPtr < zeroGroupNodes.size() && zeroGroupNodes[zeroGroupNodesPtr] == v) {
                            nextSelectedNodes.push_back(v);
                            ++zeroGroupNodesPtr;
                            continue;
                        }
                        if (v->Weight > 0) {
                            nextSelectedNodes.push_back(v);
                        }
                    }
                    if (nextSelectedNodes.size() == selectedNodes.size()) {
                        break;
                    }
                    selectedNodes.swap(nextSelectedNodes);
                }
                for (TPrefixTreeNode<LevelSize>* v : selectedNodes) {
                    TVec prefix = TVec(ui32(0));
                    TPrefixTreeNode<LevelSize>* u = v;
                    for (ui32 lev = 0; lev < v->Level; ++lev) {
                        TPrefixTreeNode<LevelSize>* prev = u;
                        u = u->Parent;
                        Y_ASSERT(u);
                        ui32 curLev = Max<ui32>();
                        for (size_t i = 0; i < LevelSize; ++i) {
                            if (u->NextNode[i] == prev) {
                                curLev = i;
                                break;
                            }
                        }
                        Y_ASSERT(curLev != Max<ui32>());
                        prefix = (prefix | (NextLevel(curLev) << lev));
                    }
                    (*groups)[PrefixGroup(prefix, VecBits - v->Level)] += v->Weight;
                }
            }

        private:
            template <size_t levelSize>
            struct TPrefixTreeNode {
                ui8 Level = 0;
                ui64 Weight = 0;
                bool UsedInCover = false;
                std::array<TPrefixTreeNode<levelSize>*, levelSize> NextNode = {{}};
                TPrefixTreeNode<levelSize>* Parent = nullptr;
                TPrefixTreeNode<levelSize>* UsedInCoverParent = nullptr;
                ui32 EulerIndex = Max<ui32>();

                TPrefixTreeNode(ui8 level, TPrefixTreeNode<levelSize>* parent = nullptr)
                    : Level(level)
                    , Parent(parent)
                {
                }
            };

            template <size_t levelSize>
            class TWeightOptimizationRangeMinimumTree {
            public:
                enum {
                    LevelSize = levelSize
                };

                TWeightOptimizationRangeMinimumTree(ui32 nodesCount, TPrefixTreeNode<LevelSize>* root)
                    : LowLevelSize_(FastClp2(nodesCount))
                    , DeltaNodePairs_(LowLevelSize_ * 2, std::make_pair(0, nullptr))
                {
                    Y_ASSERT(root);
                    Y_ASSERT(!root->UsedInCover);
                    ui32 eulerIndex = 0;
                    root->UsedInCover = true;
                    DfsInit(root, root, eulerIndex);
                    Y_ASSERT(eulerIndex == nodesCount);
                    InitInternal();
                }

                TPrefixTreeNode<LevelSize>* MaxDeltaNode() const {
                    return DeltaNodePairs_[1].second;
                }

                void IncludeInCover(TPrefixTreeNode<LevelSize>* v) {
                    Y_ASSERT(v);
                    Y_ASSERT(!v->UsedInCover);
                    v->UsedInCover = true;

                    TPrefixTreeNode<LevelSize>* u = v->Parent;
                    Y_ASSERT(u);
                    while (!u->UsedInCover) {
                        Y_ASSERT(u->Weight >= v->Weight);
                        u->Weight -= v->Weight;
                        UpdateDelta(u);
                        u = u->Parent;
                        Y_ASSERT(u);
                    }
                    Y_ASSERT(u->Weight >= v->Weight);
                    u->Weight -= v->Weight;

                    Y_ASSERT(v->EulerIndex < LowLevelSize_);
                    ui32 index = v->EulerIndex + LowLevelSize_;
                    Y_ASSERT(DeltaNodePairs_[index].second == v);
                    DeltaNodePairs_[index] = std::make_pair(0, nullptr);
                    UpNewDelta(index, v);

                    DfsChangeUsedInCoverParent(v, v);
                }

            private:
                static ui64 NodeDelta(TPrefixTreeNode<LevelSize>* v) {
                    Y_ASSERT(v->UsedInCoverParent);
                    Y_ASSERT(v->UsedInCoverParent->UsedInCover);
                    Y_ASSERT(v->Level > v->UsedInCoverParent->Level);
                    return v->Weight * (v->Level - v->UsedInCoverParent->Level);
                }

                void InitNode(TPrefixTreeNode<LevelSize>* v) {
                    Y_ASSERT(v->EulerIndex < LowLevelSize_);
                    ui32 index = v->EulerIndex + LowLevelSize_;
                    Y_ASSERT(!(DeltaNodePairs_[index].second));
                    if (!v->UsedInCover) {
                        DeltaNodePairs_[index] = std::make_pair(NodeDelta(v), v);
                    }
                }

                void InitInternal() {
                    for (ui32 i = LowLevelSize_ - 1; i > 0; --i) {
                        RecalcInternalNode(i);
                    }
                }

                void UpdateDelta(TPrefixTreeNode<LevelSize>* v) {
                    Y_ASSERT(!v->UsedInCover);
                    Y_ASSERT(v->EulerIndex < LowLevelSize_);
                    ui32 index = v->EulerIndex + LowLevelSize_;
                    Y_ASSERT(DeltaNodePairs_[index].second == v);
                    ui64 newDelta = NodeDelta(v);
                    if (newDelta == DeltaNodePairs_[index].first) {
                        return;
                    }
                    Y_ASSERT(newDelta < DeltaNodePairs_[index].first);
                    DeltaNodePairs_[index].first = newDelta;
                    UpNewDelta(index, v);
                }

                void DfsInit(TPrefixTreeNode<LevelSize>* v, TPrefixTreeNode<LevelSize>* root, ui32& eulerIndex) {
                    Y_ASSERT(v == root || !v->UsedInCover);
                    Y_ASSERT(v->EulerIndex == Max<ui32>());
                    v->EulerIndex = eulerIndex++;
                    v->UsedInCoverParent = root;
                    for (size_t i = 0; i < LevelSize; ++i) {
                        if (v->NextNode[i]) {
                            DfsInit(v->NextNode[i], root, eulerIndex);
                            v->Weight += v->NextNode[i]->Weight;
                        }
                    }
                    InitNode(v);
                }

                void DfsChangeUsedInCoverParent(TPrefixTreeNode<LevelSize>* v, TPrefixTreeNode<LevelSize>* usedInCoverParent) {
                    v->UsedInCoverParent = usedInCoverParent;
                    if (v != usedInCoverParent) {
                        UpdateDelta(v);
                    }
                    for (size_t i = 0; i < LevelSize; ++i) {
                        if (v->NextNode[i] && !v->NextNode[i]->UsedInCover) {
                            DfsChangeUsedInCoverParent(v->NextNode[i], usedInCoverParent);
                        }
                    }
                }

                void UpNewDelta(ui32 index, TPrefixTreeNode<LevelSize>* v) {
                    while (index > 1) {
                        index /= 2;
                        if (DeltaNodePairs_[index].second != v) {
                            return;
                        }
                        RecalcInternalNode(index);
                    }
                }

                void RecalcInternalNode(ui32 index) {
                    Y_ASSERT(index > 0);
                    // Let's do it deterministically (some tests may fail otherwise)
                    if (DeltaNodePairs_[index * 2].first == DeltaNodePairs_[index * 2 + 1].first) {
                        DeltaNodePairs_[index] = DeltaNodePairs_[index * 2];
                    } else {
                        DeltaNodePairs_[index] = Max(DeltaNodePairs_[index * 2], DeltaNodePairs_[index * 2 + 1]);
                    }
                }

                ui32 LowLevelSize_ = 0;
                TVector<std::pair<ui64, TPrefixTreeNode<LevelSize>*>> DeltaNodePairs_;
            };

            static ui32 GetLevel(const TVec& data, size_t lev) {
                Y_ASSERT(lev < VecBits);
                TVec level = (data >> (VecBits - lev - 1)) & TVec(1);
                ui32 elems[VecSize];
                level.Store(elems);
                ui32 result = 0;
                for (size_t i = 0; i < VecSize; ++i) {
                    Y_ASSERT(elems[i] < 2);
                    result = (result << 1) | elems[i];
                }
                return result;
            }

            static TVec NextLevel(ui32 level) {
                ui32 elems[VecSize];
                for (size_t i = 0; i < VecSize; ++i) {
                    elems[VecSize - i - 1] = (level & 1);
                    level >>= 1;
                }
                return TVec(elems);
            }

            static TVec NextPrefix(const TVec& prefix, ui32 level) {
                return (prefix << 1) | NextLevel(level);
            }

            enum EStage {
                Sampling,
                Enrichment,
                Finished,
            };

            TMemoryPool Pool_;
            ui32 NodesCount_ = 0;
            TPrefixTreeNode<LevelSize>* Root_ = nullptr;
            EStage Stage_ = Sampling;
        };

    }
}

#pragma once

#include "utility.h"

#include <util/generic/fwd.h>
#include <util/generic/vector.h>
#include <util/memory/pool.h>

namespace NOffroad {
    namespace NPrivate {
        template <class Vec>
        struct TDefaultVecTraitsDP {
            static bool ValidPrefix(const Vec& /*vec*/) {
                return true;
            }
        };
        template <class Vec, size_t vecSize, size_t vecBits, class VecTraits = TDefaultVecTraitsDP<Vec>>
        class TDpPrefixGroupBuilder {
        public:
            using TVec = Vec;
            enum {
                VecSize = vecSize,
                VecBits = vecBits,
                LevelSize = (ui32(1) << vecSize)
            };

            TDpPrefixGroupBuilder()
                : Pool_(1000 * sizeof(TPrefixTreeNode<LevelSize>), TMemoryPool::TLinearGrow::Instance())
                , NodesCount_(1)
                , Root_(Pool_.New<TPrefixTreeNode<LevelSize>>(0, 0))
            {
                Add(TVec(ui32(0)), 0);
            }

            bool IsFinished() const {
                return IsFinished_;
            }

            void Add(const TVec& data, size_t count = 1) {
                Y_VERIFY(IsFinished_ == false);
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
                        current->NextNode[level] = Pool_.New<TPrefixTreeNode<LevelSize>>(current->Level + 1, NodesCount_ - 1, current);
                    }
                    current = current->NextNode[level];
                }
                current->Weight += count;
            }

            void Reset() {
                Pool_.Clear();
                SelectedNodes_.clear();
                Dp_.clear();
                PrefDps_.clear();
                NodesCount_ = 1;
                Root_ = Pool_.New<TPrefixTreeNode<LevelSize>>(0, 0);
                IsFinished_ = false;
                Add(TVec(ui32(0)), 0);
            }

            template <class PrefixGroup, class WeightedPrefixGroupSet>

            void BuildGroups(ui32 bits, ui32 groupsCount, WeightedPrefixGroupSet* groups) {
                Y_VERIFY(IsFinished_ != true);
                IsFinished_ = true;
                Y_ENSURE(groupsCount > 0);
                Y_ENSURE(bits + 1 <= groupsCount);
                Y_ENSURE(bits <= VecBits);
                MaxMarkedNodesCnt_ = groupsCount;
                groups->clear();
                TPrefixTreeNode<LevelSize>* current = Root_;
                current->ZeroGroup = true;
                for (size_t i = 0; i <= VecBits; ++i) {
                    if (VecBits - i <= bits) {
                        current->ZeroGroup = true;
                        (*groups)[PrefixGroup(TVec(ui32(0)), VecBits - i)] = Max<ui64>() / 2;
                    }
                    if (i < VecBits) {
                        Y_ENSURE(current->NextNode[0]);
                        current = current->NextNode[0];
                    }
                }
                Dp_.clear();
                PrefDps_.clear();
                Dp_.resize(NodesCount_);
                PrefDps_.resize(NodesCount_);
                SelectedNodes_.clear();
                ui32 cntOfLeavesAndObligatoryNodes = CalcDp(Root_);
                cntOfLeavesAndObligatoryNodes = Min(cntOfLeavesAndObligatoryNodes, groupsCount);
                RestoreDp(Root_, 0, cntOfLeavesAndObligatoryNodes);
                Y_VERIFY(SelectedNodes_.size() <= groupsCount);
                current = Root_;
                for (size_t i = 0; i <= VecBits; ++i) {
                    if (current->ZeroGroup) {
                        Y_ENSURE(current->InCover);
                    }
                    if (i < VecBits) {
                        Y_ENSURE(current->NextNode[0]);
                        current = current->NextNode[0];
                    }
                }
                for (TPrefixTreeNode<LevelSize>* v : SelectedNodes_) {
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
            TMemoryPool Pool_;
            ui32 MaxMarkedNodesCnt_ = 0;
            ui32 NodesCount_ = 0;
            bool IsFinished_ = false;
            TVector<TVector<TVector<ui32>>> Dp_;
            TVector<TVector<TVector<ui32>>> PrefDps_;

            template <size_t LevelSize>
            struct TPrefixTreeNode {
                ui8 Level = 0;
                ui32 Id = 0;
                ui32 Weight = 0;
                bool ZeroGroup = false;
                bool InCover = false;
                std::array<TPrefixTreeNode<LevelSize>*, LevelSize> NextNode = {{}};
                TPrefixTreeNode<LevelSize>* Parent = nullptr;
                TPrefixTreeNode<LevelSize>* CompressedTrieLink = nullptr;
                ui32 MaxDistToDeepestMarkedAncestor = 0;
                ui32 NumLeavesAndObligatoryNodesInSubTree = 0;

                TPrefixTreeNode(ui8 level, i32 id, TPrefixTreeNode<LevelSize>* parent = nullptr)
                    : Level(level)
                    , Id(id)
                    , Parent(parent)
                {
                }
            };

            TPrefixTreeNode<LevelSize>* Root_ = nullptr;
            TVector<TPrefixTreeNode<LevelSize>*> SelectedNodes_;

            static ui32 GetLevel(const TVec& data, size_t lev) {
                Y_ASSERT(lev < VecBits);
                TVec level = (data >> (VecBits - lev - 1)) & TVec(1);
                ui32 elems[VecSize] = {};
                level.Store(elems);
                ui32 result = 0;
                for (size_t i = 0; i < VecSize; ++i) {
                    Y_ASSERT(elems[i] < 2);
                    result = (result << 1) | elems[i];
                }
                return result;
            }

            static TVec NextLevel(ui32 level) {
                ui32 elems[VecSize] = {};
                for (size_t i = 0; i < VecSize; ++i) {
                    elems[VecSize - i - 1] = (level & 1);
                    level >>= 1;
                }
                return TVec(elems);
            }

            static TVec NextPrefix(const TVec& prefix, ui32 level) {
                return (prefix << 1) | NextLevel(level);
            }

            const i32 INF = 1000000000;

            template <typename T1, typename T2>
            static inline void relax(T1& x, const T2 y) {
                x = (x > y ? y : x);
            }

            ui32 CalcDp(TPrefixTreeNode<LevelSize>* v) {
                ui32 cntOfLeavesAndObligatoryNodes = 0;
                TVector<std::pair<i32, i32>> optimalOrderForMinConvolution;
                ui32 degree = 0;
                for (size_t i = 0; i < LevelSize; ++i) {
                    if (v->NextNode[i]) {
                        i32 sonsCntOfLeavesAndObligatoryNodes = CalcDp(v->NextNode[i]);
                        cntOfLeavesAndObligatoryNodes += sonsCntOfLeavesAndObligatoryNodes;
                        ++degree;
                        optimalOrderForMinConvolution.emplace_back(sonsCntOfLeavesAndObligatoryNodes, i);
                    }
                }
                if (cntOfLeavesAndObligatoryNodes == 0) {
                    cntOfLeavesAndObligatoryNodes = 1;
                } else {
                    if (v->ZeroGroup) {
                        ++cntOfLeavesAndObligatoryNodes;
                    }
                }
                v->NumLeavesAndObligatoryNodesInSubTree = cntOfLeavesAndObligatoryNodes;
                if (degree == 1 && !v->ZeroGroup) {
                    v->CompressedTrieLink = v->NextNode[optimalOrderForMinConvolution[0].second]->CompressedTrieLink;
                    return cntOfLeavesAndObligatoryNodes;
                }
                Sort(optimalOrderForMinConvolution);
                v->CompressedTrieLink = v;
                v->MaxDistToDeepestMarkedAncestor = (v->ZeroGroup ? 0 : v->Level);
                Dp_[v->Id].resize(v->MaxDistToDeepestMarkedAncestor + 1);
                ui32 maximumReasonableCntOfMarkedNodes = Min(MaxMarkedNodesCnt_, cntOfLeavesAndObligatoryNodes);
                for (ui32 h = 0; h <= v->MaxDistToDeepestMarkedAncestor; ++h) {
                    Dp_[v->Id][h].resize(2);
                    for (i32 i = 0; i < 2; ++i) {
                        Dp_[v->Id][h][i] = INF;
                    }
                    if (h == 0) {
                        Dp_[v->Id][h][1] = 0;
                    } else {
                        Dp_[v->Id][h][0] = h * v->Weight;
                    }
                }
                ui32 maxCntOfMarkedNodes = 2;
                for (const std::pair<i32, i32>& i : optimalOrderForMinConvolution) {
                    if (v->NextNode[i.second]) {
                        TPrefixTreeNode<LevelSize>* u = v->NextNode[i.second]->CompressedTrieLink;
                        ui32 distFromMarkedAncestor = u->Level - v->Level;
                        ui32 newMaxCntOfMarkedNodes = Min(maxCntOfMarkedNodes + static_cast<i32>(Dp_[u->Id][0].size()) - 1, maximumReasonableCntOfMarkedNodes + 1);
                        TVector<ui32> buffDp;
                        buffDp.resize(newMaxCntOfMarkedNodes);
                        PrefDps_[u->Id].resize(v->MaxDistToDeepestMarkedAncestor + 1);
                        for (ui32 h = 0; h <= v->MaxDistToDeepestMarkedAncestor; ++h) {
                            PrefDps_[u->Id][h] = Dp_[v->Id][h];
                            Fill(buffDp.begin(), buffDp.end(), INF);
                            i32 sonsH = Min(h + distFromMarkedAncestor, u->MaxDistToDeepestMarkedAncestor);
                            for (ui32 j = 0; j < maxCntOfMarkedNodes; ++j) {
                                for (ui32 k = 0; k < Min((ui32)Dp_[u->Id][0].size(), newMaxCntOfMarkedNodes - j); ++k) {
                                    relax(buffDp[j + k], Dp_[v->Id][h][j] + Dp_[u->Id][sonsH][k]);
                                }
                            }
                            Dp_[v->Id][h] = buffDp;
                        }
                        maxCntOfMarkedNodes = newMaxCntOfMarkedNodes;
                    }
                }
                for (ui32 i = 0; i <= v->MaxDistToDeepestMarkedAncestor; ++i) {
                    for (ui32 j = 0; j <= maximumReasonableCntOfMarkedNodes; ++j) {
                        relax(Dp_[v->Id][i][j], Dp_[v->Id][0][j]);
                    }
                }
                return cntOfLeavesAndObligatoryNodes;
            }

            void RestoreDp(TPrefixTreeNode<LevelSize>* v, ui32 h, ui32 curCnt) {
                if (curCnt == 0) {
                    return;
                }
                if (h == 0) {
                    v->InCover = true;
                    SelectedNodes_.push_back(v);
                }
                TVector<std::pair<ui32, ui32>> optimalOrderForMinConvolution;
                ui32 degree = 0;
                for (size_t i = 0; i < LevelSize; ++i) {
                    if (v->NextNode[i]) {
                        ui32 sz = v->NextNode[i]->NumLeavesAndObligatoryNodesInSubTree;
                        optimalOrderForMinConvolution.emplace_back(sz, i);
                        ++degree;
                    }
                }
                if (degree == 0) {
                    return;
                }
                Sort(optimalOrderForMinConvolution);
                ui32 curVal = Dp_[v->Id][h][curCnt];
                ui32 ourMarkedCnt = (v->InCover ? 1 : 0);
                for (i32 id = static_cast<i32>(degree) - 1; id >= 0; --id) {
                    TPrefixTreeNode<LevelSize>* u = v->NextNode[optimalOrderForMinConvolution[id].second]->CompressedTrieLink;
                    ui32 distFromMarkedAncestor = u->Level - v->Level;
                    ui32 sonsH = Min(h + distFromMarkedAncestor, u->MaxDistToDeepestMarkedAncestor);
                    i32 optimalPrefCntOfMarkedNodes = -1;
                    for (ui32 j = static_cast<ui32>(Max(static_cast<i32>(ourMarkedCnt), static_cast<i32>(curCnt) - static_cast<i32>(Dp_[u->Id][sonsH].size()) + 1)); j < Min(curCnt + 1, static_cast<ui32>(PrefDps_[u->Id][h].size())); ++j) {
                        if (PrefDps_[u->Id][h][j] + Dp_[u->Id][sonsH][curCnt - j] == curVal) {
                            optimalPrefCntOfMarkedNodes = j;
                            break;
                        }
                    }
                    Y_VERIFY(optimalPrefCntOfMarkedNodes != -1);
                    ui32 cntOfMarkedNodesInSonsSubtree = curCnt - optimalPrefCntOfMarkedNodes;
                    if (Dp_[u->Id][sonsH][cntOfMarkedNodesInSonsSubtree] == Dp_[u->Id][0][cntOfMarkedNodesInSonsSubtree]) {
                        sonsH = 0;
                    }
                    RestoreDp(u, sonsH, cntOfMarkedNodesInSonsSubtree);
                    curCnt = optimalPrefCntOfMarkedNodes;
                    curVal = PrefDps_[u->Id][h][optimalPrefCntOfMarkedNodes];
                }
                Y_VERIFY(curCnt == ourMarkedCnt);
            }
        };
    }
}

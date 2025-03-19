#include "sequence_accessors.h"

#include "block_accessors.h"
#include "size_limits.h"

namespace {
    using namespace NReqBundle;

    inline i64 GetNumericType(TConstSequenceElemAcc x) {
        return x.HasBlock() ? 2 : (x.HasBinary() ? 1 : 0);
    }

    inline i64 CompareInternal(TConstSequenceElemAcc x, TConstSequenceElemAcc y) {
        if (x.IsValid() && y.IsValid()) {
            i64 xType = GetNumericType(x);
            i64 yType = GetNumericType(y);

            if (xType != yType || 0 == xType) {
                return xType - yType;
            }
            if (2 == xType) {
                Y_ASSERT(x.HasBlock() && y.HasBlock());
                return NReqBundle::Compare(x.GetBlock(), y.GetBlock());
            } else {
                Y_ASSERT(x.HasBinary() && y.HasBinary());
                ui64 xHash = x.GetBinary().GetHash();
                ui64 yHash = y.GetBinary().GetHash();

                if (xHash == yHash) {
                    TBlob xBlob = x.GetBinary().GetData();
                    TBlob yBlob = y.GetBinary().GetData();

                    size_t xSize = xBlob.Size();
                    size_t ySize = yBlob.Size();

                    Y_ASSERT(xSize < static_cast<size_t>(Max<i64>()));
                    Y_ASSERT(ySize < static_cast<size_t>(Max<i64>()));

                    return static_cast<i64>(xSize) - static_cast<i64>(ySize);
                } else {
                    return xHash < yHash ? -1 : 1;
                }
            }
        } else {
            return static_cast<int>(x.IsValid()) - static_cast<int>(y.IsValid());
        }
    }
} // namespace

namespace NReqBundle {
    int Compare(TConstSequenceElemAcc x, TConstSequenceElemAcc y) {
        i64 res = CompareInternal(x, y);
        return res < 0 ? -1 : (res > 0 ? 1 : 0);
    }

    namespace NDetail {
        EValidType IsValidMatch(TConstMatchAcc match, TConstRequestAcc request, TConstSequenceAcc seq)
        {
            if (!(match.GetWordIndexFirst() < request.GetNumWords()
                && match.GetWordIndexLast() < request.GetNumWords()
                && match.GetWordIndexFirst() <= match.GetWordIndexLast()
                && match.GetNumWords() > 0
                && match.GetBlockIndex() < seq.GetNumElems()))
            {
                return VT_FALSE;
            }

            if (match.GetType() != TMatch::OriginalWord) {
                return VT_TRUE;
            }

            const size_t blockIndex = match.GetBlockIndex();

            if (!seq.HasBlock(blockIndex)) {
                return VT_UNKNOWN;
            }
            auto block = seq.GetBlock(blockIndex);
            if (!(block.GetNumWords() == match.GetNumWords())){
                return VT_FALSE;
            }
            return VT_TRUE;
        }

        EValidType IsValidConstraint(TConstConstraintAcc constraint, TConstSequenceAcc seq)
        {
            const TVector<size_t>& blockIndices = constraint.GetBlockIndices();

            if (blockIndices.empty()) {
                return VT_FALSE;
            }

            EValidType res = VT_TRUE;
            for (const size_t blockIdx : blockIndices) {
                if (blockIdx >= seq.GetNumElems()) {
                    return VT_FALSE;
                }

                TConstSequenceElemAcc elem = seq.GetElem(blockIdx);
                if (!elem.HasBlock()) {
                    if (!elem.HasBinary()) {
                        return VT_FALSE;
                    }
                    res = res && VT_UNKNOWN;
                }
            }

            return res;
        }

        EValidType IsValidTrCompatibilityInfo(TConstRequestAcc request)
        {
            if (request.GetTrCompatibilityInfo().Empty()) {
                return VT_TRUE;
            }
            const bool needTrCompatibilityInfo = AnyOf(request.GetFacets().GetEntries(), [](TConstFacetEntryAcc entry) { return entry.NeedTrCompatibilityInfo(); });
            const bool isOriginalRequest =  AnyOf(request.GetFacets().GetEntries(), [](TConstFacetEntryAcc entry) { return entry.IsOriginalRequest(); });

            if (!needTrCompatibilityInfo) {
                return VT_FALSE;
            }
            const TRequestTrCompatibilityInfo& info = *request.GetTrCompatibilityInfo();
            // for OriginalSynonymsRequest we skip some checks because it does not contain OriginalWords at all
            if (isOriginalRequest) {
                if (info.MainPartsWordMasks.size() + info.MarkupPartsBestFormClasses.size() != request.GetNumMatches()) {
                    return VT_FALSE;
                }
                for (size_t i : xrange(request.GetNumMatches())) {
                    auto match = request.GetMatch(i);
                    if (i < info.MainPartsWordMasks.size()) {
                        if (match.GetType() != EMatchType::OriginalWord) {
                            return VT_FALSE;
                        }
                    } else if (match.GetType() != EMatchType::Synonym) {
                        return VT_FALSE;
                    }
                }
            } else {
                for (size_t i : xrange(request.GetNumMatches())) {
                    auto match = request.GetMatch(i);
                    if (match.GetType() != EMatchType::Synonym) {
                        return VT_FALSE;
                    }
                }
            }
            return VT_TRUE;
        }

        EValidType IsValidRequest(TConstRequestAcc request, TConstSequenceAcc seq, bool validateTrCompatibilityInfo)
        {
            if (request.GetNumWords() == 0) {
                return VT_FALSE;
            }
            if (request.GetNumWords() > TSizeLimits::MaxNumWordsInRequest) {
                return VT_FALSE;
            }
            if (request.GetNumMatches() > TSizeLimits::MaxNumMatches) {
                return VT_FALSE;
            }
            if (request.GetFacets().GetNumEntries() > TSizeLimits::MaxNumFacets) {
                return VT_FALSE;
            }

            EValidType res = VT_TRUE;
            for (auto entry : request.GetFacets().GetEntries()) {
                const EValidType facetRes = IsValidFacet(entry);
                if (VT_FALSE == facetRes) {
                    return VT_FALSE;
                }
                res = res && facetRes;
            }

            for (auto match : request.GetMatches()) {
                const EValidType matchRes = IsValidMatch(match, request, seq);
                if (VT_FALSE == matchRes) {
                    return VT_FALSE;
                }
                res = res && matchRes;
            }

            if (validateTrCompatibilityInfo) {
                EValidType trCompatibilityInfoRes = IsValidTrCompatibilityInfo(request);
                if (VT_FALSE == trCompatibilityInfoRes) {
                    return VT_FALSE;
                }
                res = res && trCompatibilityInfoRes;
            }

            return res;
        }

        EValidType IsValidSequence(TConstSequenceAcc seq, const TValidConstraints& constr) {
            if (constr.NeedNonEmpty && 0 == seq.GetNumElems()) {
                return VT_FALSE;
            }

            EValidType res = VT_TRUE;
            for (auto elem : seq.GetElems()) {
                if (constr.NeedBlocks && !elem.HasBlock()) {
                    return VT_FALSE;
                }
                if (constr.NeedBinaries && !elem.HasBinary()) {
                    return VT_FALSE;
                }
                if (!elem.HasBlock()) {
                    if (!elem.HasBinary()) {
                        return VT_FALSE;
                    }
                    res = res && VT_UNKNOWN;
                    continue;
                }
                const EValidType blockRes = IsValidBlock(elem.GetBlock());
                if (VT_FALSE == blockRes) {
                    return VT_FALSE;
                }
                res = res && blockRes;
            }
            return res;
        }
    } // NDetail
} // NReqBundle

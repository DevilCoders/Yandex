#pragma once

#include "constraint_accessors.h"
#include "request_accessors.h"
#include "sequence_contents.h"
#include "reqbundle_fwd.h"

namespace NReqBundle {
    template <typename DataType>
    class TSequenceElemAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TSequenceElemAccBase() = default;
        TSequenceElemAccBase(const TSequenceElemAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TSequenceElemAccBase(const TConstSequenceElemAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TSequenceElemAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        bool HasBlock() const {
            return !!Contents().Block;
        }
        TConstBlockAcc GetBlock() const {
            return *Contents().Block;
        }
        TBlockAcc Block() const {
            return *Contents().Block;
        }
        void DiscardBlock() const {
            Y_ASSERT(Contents().Binary);
            Contents().Block = TBlockPtr();
        }
        void PrepareBlock(TReqBundleDeserializer& deser) const {
            if (!Contents().Block) {
                NDetail::PrepareBlock(Contents(), deser);
            }
        }

        bool HasBinary() const {
            return !!Contents().Binary;
        }
        TConstBinaryBlockAcc GetBinary() const {
            return *Contents().Binary;
        }
        TBinaryBlockAcc Binary() const {
            return *Contents().Binary;
        }
        void DiscardBinary() const {
            Y_ASSERT(Contents().Block);
            Contents().Binary = TBinaryBlockPtr();
        }
        void PrepareBinary(TReqBundleSerializer& ser) const {
            if (!Contents().Binary) {
                NDetail::PrepareBinary(Contents(), ser);
            }
        }
    };

    template <typename DataType>
    class TSequenceAccBase
        : public NDetail::TLightAccessor<DataType>
    {
    public:
        using NDetail::TLightAccessor<DataType>::Contents;

        TSequenceAccBase() = default;
        TSequenceAccBase(const TSequenceAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TSequenceAccBase(const TConstSequenceAcc& other)
            : NDetail::TLightAccessor<DataType>(other)
        {}
        TSequenceAccBase(DataType& data)
            : NDetail::TLightAccessor<DataType>(data)
        {}

        size_t GetNumElems() const {
            return Contents().Elems.size();
        }
        TSequenceElemAcc Elem(size_t elemIndex) const {
            return TSequenceElemAcc(Contents().Elems[elemIndex]);
        }
        TSequenceElemAcc Elem(TConstMatchAcc match) const {
            return Elem(match.GetBlockIndex());
        }
        TConstSequenceElemAcc GetElem(size_t elemIndex) const {
            return TConstSequenceElemAcc(Contents().Elems[elemIndex]);
        }
        TConstSequenceElemAcc GetElem(TConstMatchAcc match) const {
            return GetElem(match.GetBlockIndex());
        }

        auto Elems() const -> decltype(NDetail::MakeAccContainer<TSequenceElemAcc>(std::declval<DataType&>().Elems)) {
            return NDetail::MakeAccContainer<TSequenceElemAcc>(Contents().Elems);
        }
        auto GetElems() const -> decltype(NDetail::MakeAccContainer<TConstSequenceElemAcc>(std::declval<DataType&>().Elems)) {
            return NDetail::MakeAccContainer<TConstSequenceElemAcc>(Contents().Elems);
        }

        bool HasBlock(size_t elemIndex) const {
            return GetElem(elemIndex).HasBlock();
        }
        TConstBlockAcc GetBlock(size_t elemIndex) const {
            return GetElem(elemIndex).GetBlock();
        }
        TBlockAcc Block(size_t elemIndex) const {
            return Elem(elemIndex).Block();
        }
        void PrepareAllBlocks(TReqBundleDeserializer& deser) const {
            for (auto elem : Elems()) {
                elem.PrepareBlock(deser);
            }
        }
        void DiscardAllBlocks() const {
            for (auto elem : Elems()) {
                elem.DiscardBlock();
            }
        }

        bool HasBinary(size_t elemIndex) const {
            return GetElem(elemIndex).HasBinary();
        }
        TConstBinaryBlockAcc GetBinary(size_t elemIndex) const {
            return GetElem(elemIndex).GetBinary();
        }
        TBinaryBlockAcc Binary(size_t elemIndex) const {
            return Elem(elemIndex).Binary();
        }
        void PrepareAllBinaries(TReqBundleSerializer& ser) const {
            for (auto elem : Elems()) {
                elem.PrepareBinary(ser);
            }
        }
        void DiscardAllBinaries() const {
            for (auto elem : Elems()) {
                elem.DiscardBinary();
            }
        }

        size_t AddElem(const TBlockPtr& block) const {
            Contents().Elems.push_back(NDetail::SequenceElem(block));
            return Contents().Elems.size() - 1;
        }
        size_t AddElem(const TBinaryBlockPtr& binary) const {
            Contents().Elems.push_back(NDetail::SequenceElem(binary));
            return Contents().Elems.size() - 1;
        }
        size_t AddElem(TConstSequenceElemAcc elem) const {
            Y_ASSERT(elem.HasBlock() || elem.HasBinary());
            Contents().Elems.push_back(NDetail::BackdoorAccess(elem));
            return Contents().Elems.size() - 1;
        }
    };

    int Compare(TConstSequenceElemAcc x, TConstSequenceElemAcc y);

    inline bool operator < (TConstSequenceElemAcc x, TConstSequenceElemAcc y) {
        return Compare(x, y) < 0;
    }

    namespace NDetail {
        EValidType IsValidMatch(TConstMatchAcc match, TConstRequestAcc request, TConstSequenceAcc seq);
        EValidType IsValidConstraint(TConstConstraintAcc constraint, TConstSequenceAcc seq);
        EValidType IsValidRequest(TConstRequestAcc request, TConstSequenceAcc seq, bool validateTrCompatibilityInfo = true);
        EValidType IsValidSequence(TConstSequenceAcc seq, const TValidConstraints& constr = {});
        EValidType IsValidTrCompatibilityInfo(TConstRequestAcc request);
    } // NDetail
} // NReqBundle

#pragma once

#include "reqbundle_iterator_fwd.h"

#include <library/cpp/wordpos/wordpos.h>

#include <util/generic/ylimits.h>
#include <util/generic/typetraits.h>

namespace NReqBundleIterator {
    class TBitBase {
    public:
        enum {
            TotalBits = 0,
        };

        ui64 Raw;

    public:
        void Clear() {
            Raw = 0;
        }
    };

    template<class TBase, size_t Bits>
    class TBitField
        : public TBase
    {
    public:
        enum {
            Offset = TBase::TotalBits,
            TotalBits = Offset + Bits,
            MaxValue = (ui64(1) << Bits) - ui64(1),
        };

        static_assert(Bits <= 32, "");
        static_assert(TotalBits <= 64, "");

        static constexpr ui64 Mask = ui64(MaxValue) << Offset;
        static constexpr ui64 UpperMask = ~((ui64(1) << Offset) - ui64(1));

    public:
        Y_FORCE_INLINE ui32 ToNumber() const {
            return (this->Raw & Mask) >> Offset;
        }
        Y_FORCE_INLINE bool Equal(TBitField right) const {
            return ((this->Raw ^ right.Raw) & Mask) == 0;
        }
        Y_FORCE_INLINE bool EqualUpper(TBitField right) const {
            return ((this->Raw ^ right.Raw) & UpperMask) == 0;
        }
        Y_FORCE_INLINE ui64 Upper() const {
            return (this->Raw & UpperMask);
        }
        Y_FORCE_INLINE void Set(ui32 val) {
            Y_ASSERT(val <= MaxValue);
            this->Raw = this->Raw & ~Mask | (ui64)val << Offset;
        }
        Y_FORCE_INLINE void SetClean(ui32 val) {
            Y_ASSERT((this->Raw & Mask) == 0);
            Y_ASSERT(val <= MaxValue);
            this->Raw |= (ui64)val << Offset;
        }
        Y_FORCE_INLINE void SetMin(TBitField right) {
            if ((this->Raw & Mask) > (right.Raw & Mask))
                this->Raw = this->Raw & ~Mask | right.Raw & Mask;
        }
        Y_FORCE_INLINE void SetMax(TBitField right) {
            if ((this->Raw & Mask) < (right.Raw & Mask))
                this->Raw = this->Raw & ~Mask | right.Raw & Mask;
        }
    };

    // Order is important
    struct TLemmId : TBitField<TBitBase, 6> {};
    struct TLowLevelFormId : TBitField<TLemmId, 14> {};
    struct TMatch : TBitField<TLowLevelFormId, 2> {};
    struct TRelev : TBitField<TMatch, 2> {};
    struct TBlockId : TBitField<TRelev, 12> {};
    struct TWordPosEnd : TBitField<TBlockId, 6> {};
    struct TWordPosBeg : TBitField<TWordPosEnd, 6> {};
    struct TBreak : TBitField<TWordPosBeg, 15> {};

    class TPosition
        : public TBreak
    {
    public:
        static const TPosition Invalid;

        Y_FORCE_INLINE ui32 Break() const {
            return TBreak::ToNumber();
        }
        Y_FORCE_INLINE ui32 WordPosBeg() const {
            return TWordPosBeg::ToNumber();
        }
        Y_FORCE_INLINE ui32 WordPosEnd() const {
            return TWordPosEnd::ToNumber();
        }
        Y_FORCE_INLINE ui32 BlockId() const {
            return TBlockId::ToNumber();
        }
        Y_FORCE_INLINE ui32 LemmId() const {
            return TLemmId::ToNumber();
        }
        /*
            LowLevelFormId is assigned based on index.
            It is unstable, can change when index is updated
            even if query and document are not affected,
            can be different in different streams (text/link/annotation).

            Use TReqBundleIterator::GetRichTreeFormId to obtain
            a stable RichTreeFormId.
            RichTreeFormId is assigned based on RichTree only.
            TFormIndexAssigner in form_index_assigner.h does the job.
            RichTreeFormId is fixed if query wizarding does not change,
            but sometimes RichTreeFormId is not defined:
            there are forms in index that are accepted but do not exist in RichTree.
            In these cases, GetRichTreeFormId() returns zero;
            numeration of normal values starts with 1.
        */
        Y_FORCE_INLINE ui32 LowLevelFormId() const {
            return TLowLevelFormId::ToNumber();
        }
        Y_FORCE_INLINE EFormClass Match() const {
            return (EFormClass)TMatch::ToNumber();
        }
        Y_FORCE_INLINE ui32 Relev() const {
            return TRelev::ToNumber();
        }

        static Y_FORCE_INLINE ui64 MaxValue() {
            return Max<ui64>();
        }

        Y_FORCE_INLINE bool Valid() const {
            return Raw != Max<ui64>();
        }
        Y_FORCE_INLINE void SetInvalid() {
            Raw = MaxValue();
        }

        operator ui64& () {
            return Raw;
        }
        operator const ui64& () const {
            return Raw;
        }
        void operator = (ui64 value) {
            Raw = value;
        }
    };
} // NReqBundleIterator

Y_DECLARE_PODTYPE(NReqBundleIterator::TPosition);

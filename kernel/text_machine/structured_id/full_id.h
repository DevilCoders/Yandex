#pragma once

#include "id_internals.h"
#include "id_traits.h"
#include "id_builder.h"
#include "id_utils.h"

namespace NStructuredId {
    // How to create new type of id
    //
    // 1. Redefine TGetIdTraitsList<EIdPartType> template
    //    for your EIdPartType enumeration.
    //
    //    See unit tests or kernel/text_machine/interface/feature_traits.h
    //    for examples.
    //
    // 2. Base id is constructed from traits list
    template <typename EIdPartType>
    using TFullIdBase = NDetail::TIdImpl<EIdPartType, TGetIdTraitsList<EIdPartType>::TResult::Id>;

    // 3. Use this class to hold full id
    template <typename EIdPartType>
    class TFullId : public TFullIdBase<EIdPartType> {
    public:
        using TPartEnum = EIdPartType;
        using TStringId = NStructuredId::TStringId<EIdPartType>;

    public:
        TFullId() = default;
        TFullId(const TFullId<EIdPartType>&) = default;

        TFullId(const TFullIdBase<EIdPartType>& other)
            : TFullIdBase<EIdPartType>(other)
        {
        }

        template <typename T, typename... Args>
        explicit Y_FORCE_INLINE TFullId(const T& arg1, const Args& ... args) {
            Set(arg1, args...);
        }

        TFullId& operator = (const TFullId<EIdPartType>&) = default;
        using TFullIdBase<EIdPartType>::operator ==;
        using TFullIdBase<EIdPartType>::operator <;
        using TFullIdBase<EIdPartType>::Hash;

        bool operator != (const TFullId<EIdPartType>& other) const {
            return !(*this == other);
        }

        template <typename ArgX, typename ArgY, typename... Args>
        Y_FORCE_INLINE void Set(const ArgX& argX, const ArgY& argY, const Args& ... args) {
            Set(argX);
            Set(argY, args...);
        }
        template <typename ArgY, typename... Args>
        Y_FORCE_INLINE void Set(const TFullId<EIdPartType>& argX, const ArgY& argY, const Args& ... args) {
            *this = argX;
            Set(argY, args...);
        }
        template <typename T>
        Y_FORCE_INLINE void Set(T value) {
            TFullIdBase<EIdPartType>::Set(value);
        }
        Y_FORCE_INLINE void Set() {
        }
    };
} // NStructuredId

template <typename EIdPartType>
inline NStructuredId::TFullId<EIdPartType> FromStringId(const NStructuredId::TStringId<EIdPartType>& strId) {
    return NStructuredId::TFullId<EIdPartType>::FromStringId(strId);
}

template <typename EIdPartType>
inline NStructuredId::TStringId<EIdPartType> ToStringId(const NStructuredId::TFullId<EIdPartType>& id) {
    return NStructuredId::TFullId<EIdPartType>::ToStringId(id);
}

template <typename EIdPartType>
struct THash<::NStructuredId::TFullId<EIdPartType>> {
    ui64 operator() (const ::NStructuredId::TFullId<EIdPartType>& x) {
        return x.Hash();
    }
};

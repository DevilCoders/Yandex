#pragma once

#include "id_internals.h"
#include "id_builder.h"

namespace NStructuredId {
    template <typename EIdPartType>
    struct TIdBuilder;

    template <typename EIdPartType>
    struct TGetIdTraitsList;

    namespace NDetail {
        template <typename EIdPartType, EIdPartType PartId, typename List>
        struct TIdAccess
            : public TIdAccess<EIdPartType, PartId, typename List::TNext>
        {};

        template <typename EIdPartType, EIdPartType PartId, typename Mixin, typename... Pairs>
        struct TIdAccess<EIdPartType, PartId,
            NDetail::TIdList<EIdPartType, NDetail::TIdPair<EIdPartType, PartId, Mixin>, Pairs...>>
        {
            using TResult = NDetail::TIdList<EIdPartType, NDetail::TIdPair<EIdPartType, PartId, Mixin>, Pairs...>;
        };

        template <typename EIdPartType, EIdPartType PartId>
        using TIdAccessResult = typename TIdAccess<EIdPartType, PartId, typename TGetIdTraitsList<EIdPartType>::TResult>::TResult;

        template <typename EIdPartType, typename SelectPartType, typename = void>
        struct TIdTraitsHelper;

        template<typename EIdPartType>
        using TFirstPartSelector = TSelectPart<EIdPartType, TGetIdTraitsList<EIdPartType>::TResult::TFirst::Id>;

        template <typename EIdPartType, EIdPartType PartId>
        struct TIdTraitsHelper<EIdPartType, TSelectPart<EIdPartType, PartId>,
            typename std::enable_if<
                !std::is_same<TFirstPartSelector<EIdPartType>, TSelectPart<EIdPartType, PartId>>::value
            >::type
        > {
            static constexpr EIdPartType Id = PartId;
            using TMixin = typename TIdAccessResult<EIdPartType, PartId>::TMixin;
            using TBase = TIdImpl<EIdPartType, TIdAccessResult<EIdPartType, PartId>::TNext::Id>;
        };

        template <typename EIdPartType>
        struct TIdTraitsHelper<EIdPartType, TFirstPartSelector<EIdPartType>> {
            static constexpr EIdPartType Id = TGetIdTraitsList<EIdPartType>::TResult::TFirst::Id;
            using TMixin = typename TIdAccessResult<EIdPartType, Id>::TMixin;
            using TBase = TNullId;
        };

        template <typename EIdPartType, EIdPartType PartId>
        struct TIdTraits : public TIdTraitsHelper<EIdPartType, TSelectPart<EIdPartType, PartId>> {};
    } // NDetail

    template <typename EIdPartType>
    using TIdBase = NDetail::TIdImpl<EIdPartType, TGetIdTraitsList<EIdPartType>::TResult::Id>;
} // NStructuredId

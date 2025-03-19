#pragma once

#include "id_internals.h"
#include "typed_param.h"

namespace NStructuredId {
    namespace NDetail {
        template <typename EIdPartType, EIdPartType PartId, typename Mixin>
        struct TIdPair {
            static const EIdPartType Id = PartId;
            using TMixin = Mixin;
        };

        template <typename EIdPartType, EIdPartType PartId, typename Mixin>
        const EIdPartType TIdPair<EIdPartType, PartId, Mixin>::Id;

        template <typename EIdPartType, typename... Pairs>
        struct TIdList;

        template <typename EIdPartType, typename Pair, typename... Pairs>
        struct TIdList<EIdPartType, Pair, Pairs...> {
            using TMixin = typename Pair::TMixin;
            using TNext = TIdList<EIdPartType, Pairs...>;
            using TFirst = typename TNext::TFirst;
            static const EIdPartType Id = Pair::Id;
        };

        template <typename EIdPartType, typename Pair, typename... Pairs>
        const EIdPartType TIdList<EIdPartType, Pair, Pairs...>::Id;

        template <typename EIdPartType, typename Pair>
        struct TIdList<EIdPartType, Pair> {
            using TMixin = typename Pair::TMixin;
            using TNext = TIdList<EIdPartType>;
            using TFirst = TIdList<EIdPartType, Pair>;
            static const EIdPartType Id = Pair::Id;
        };

        template <typename EIdPartType, typename Pair>
        const EIdPartType TIdList<EIdPartType, Pair>::Id;

        template <typename EIdPartType>
        struct TIdList<EIdPartType> {
        };
    } // NDetail

    template <typename EIdPartType>
    struct TIdBuilder {
        template <EIdPartType PartId, typename Mixin>
        using TPair = NDetail::TIdPair<EIdPartType, PartId, Mixin>;

        template <typename... Pairs>
        using TListType = NDetail::TIdList<EIdPartType, Pairs...>;
    };

    template <typename EIdPartType>
    struct TGetIdTraitsList {
        using TResult = typename TIdBuilder<EIdPartType>::template TListType<>;
    };
} // NStructuredId

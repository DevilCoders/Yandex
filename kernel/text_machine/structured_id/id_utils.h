#pragma once

namespace NStructuredId {
    namespace NDetail {
        struct TIsEqual {
            template <typename ValueType>
            inline bool IsSubValue(
                const ValueType& x,
                const ValueType& y)
            {
                return x == y;
            }
        };

        template <typename IdType, typename FuncType>
        struct TSubIdAccumulator {
            using TPartEnum = typename IdType::TPartEnum;

            const IdType& IdX;
            const IdType& IdY;
            FuncType Func;

            bool Result = true;

            TSubIdAccumulator(
                const IdType& idX,
                const IdType& idY,
                const FuncType& func)
                : IdX(idX)
                , IdY(idY)
                , Func(func)
            {}

            template <TPartEnum PartId>
            void Do() {
                Result = Result && (!IdY.template IsValid<PartId>()
                    || (IdX.template IsValid<PartId>()
                        && Func.IsSubValue(IdX.template Get<PartId>(), IdY.template Get<PartId>())));
            }
        };
    }

    // Checks that idX is sub-id of idY.
    // I.e. for each component idX and idY either have equal values, or
    // idY is unset.
    template <typename IdType, typename FuncType = NDetail::TIsEqual>
    inline bool IsSubId(
        const IdType& idX,
        const IdType& idY,
        const FuncType& func = FuncType{})
    {
        NDetail::TSubIdAccumulator<IdType, FuncType> acc(idX, idY, func);
        IdType::ForEach(acc);
        return acc.Result;
    }
}

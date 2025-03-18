#pragma once

#include <type_traits>

namespace NOffroad {
    template <class Writer0, class Writer1>
    struct TFinisher {
        using TFinishResult0 = decltype(std::declval<Writer0*>()->Finish());
        using TFinishResult1 = decltype(std::declval<Writer1*>()->Finish());

        enum {
            FinishReturnsVoid0 = std::is_same<TFinishResult0, void>::value,
            FinishReturnsVoid1 = std::is_same<TFinishResult1, void>::value,
        };

        static_assert(FinishReturnsVoid0 || FinishReturnsVoid1, "Both Finish calls return non-void, you're likely using two samplers simultaneously.");

        using result_type = std::conditional_t<FinishReturnsVoid1, TFinishResult0, TFinishResult1>;

        template <bool value>
        using TBool = std::integral_constant<bool, value>;

        result_type operator()(Writer0* writer0, Writer1* writer1) const {
            return operator()(writer0, writer1, TBool<FinishReturnsVoid0>());
        }

        TFinishResult0 operator()(Writer0* writer0, Writer1* writer1, TBool<false>) const {
            writer1->Finish();
            return writer0->Finish();
        }

        TFinishResult1 operator()(Writer0* writer0, Writer1* writer1, TBool<true>) const {
            writer0->Finish();
            return writer1->Finish();
        }
    };

}

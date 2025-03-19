#pragma once

#include <kernel/lemmer/core/handler.h>
#include <kernel/lemmer/core/lemmer.h>

namespace NLemmer {
    struct TLemmaOrder {
        inline bool operator()(const TYandexLemma& l1, const TYandexLemma& l2) const {
            if (l1.GetTokenPos() != l2.GetTokenPos()) {
                return l1.GetTokenPos() < l2.GetTokenPos();
            }

            return l1.GetTokenSpan() > l2.GetTokenSpan();
        }

        inline bool AreEqual(const TYandexLemma& l1, const TYandexLemma& l2) const {
            return l1.GetTokenPos() == l2.GetTokenPos() && l1.GetTokenSpan() == l2.GetTokenSpan();
        }
    };

    // most probable result becomes first after sorting
    // MinElement(results, TLemmerResultOrderGreater()) will have max probable score
    struct TLemmerResultOrderGreater {
        TLemmaOrder LemmaOrder;

        inline bool operator()(const TLemmerResult& d1, const TLemmerResult& d2) const {
            if (LemmaOrder.AreEqual(d1.first, d2.first)) {
                return d1.second > d2.second;
            }
            return LemmaOrder(d1.first, d2.first);
        }
    };
}

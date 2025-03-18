#pragma once

#include "solution.h"
#include "rank.h"

#include <util/system/yassert.h>

namespace NSolveAmbig {
    namespace NImpl {
        struct TSolutionCoverage {
            static inline size_t Of(const TSolution& solution) {
                return solution.Coverage;
            }
        };

        struct TSolutionCount {
            static inline size_t Of(const TSolution& solution) {
                return solution.Positions.size();
            }
        };

        struct TSolutionWeight {
            static inline double Of(const TSolution& solution) {
                return solution.Weight;
            }
        };

        template <typename TValue>
        inline int Compare(const TSolution& l, const TSolution& r) {
            return (TValue::Of(l) == TValue::Of(r)) ? 0 : ((TValue::Of(l) > TValue::Of(r)) ? 1 : -1);
        }

        class TSolutionOrder {
        private:
            TRankMethod RankMethod;

        public:
            explicit TSolutionOrder(const TRankMethod& rankMethod)
                : RankMethod(rankMethod)
            {
            }

            inline bool operator()(const TSolution& l, const TSolution& r) const {
                for (auto iRankCheck : RankMethod) {
                    int result = 0;
                    switch (iRankCheck) {
                        case RC_G_COVERAGE:
                            result = Compare<TSolutionCoverage>(l, r);
                            break;
                        case RC_L_COUNT:
                            result = -Compare<TSolutionCount>(l, r);
                            break;
                        case RC_G_WEIGHT:
                            result = Compare<TSolutionWeight>(l, r);
                            break;
                        default:
                            Y_FAIL("Unsupported ranking check");
                    }
                    if (result) {
                        return result > 0;
                    }
                }

                return false;
            }

            inline bool operator()(const TSolutionPtr& l, const TSolutionPtr& r) const {
                Y_ASSERT(l.Get());
                Y_ASSERT(r.Get());
                return operator()(*l, *r);
            }
        };

    }

}

#pragma once

#include "factors.h"
#include <util/generic/ymath.h>

namespace NMango
{

/*
 * Зависимость, какие мега-факторы от каких линеек зависят:
 *
 * SIMPLE_RELEVANCE               -  GROUP_W<i>_RELEVANCE
 * SIMPLE_INTERESTINGNESS         -  GROUP_W<i>_QUOTE_REL, GROUP_W<i>_OTHER
 * SIMPLE_STATIC_INTERESTINGNESS  -  GROUP_QUOTE_STATIC
 * SIMPLE_TRASHLESS               -  GROUP_W<i>_QUOTE_REL
 * SIMPLE_STATIC_TRASHLESS        -  GROUP_QUOTE_STATIC, GROUP_OTHER_STATIC
 *
 */

    inline float Approximate(const float* values, size_t size, float step, float value)
    {
        if (IsNan(value) || value < 0.0f) {
            return 1.0f;
        }

        value /= step;
        size_t bucket = floor(value);
        float delta = value - bucket;
        bucket = Min(bucket, size - 2);

        return values[bucket] * (1.0f - delta) + values[bucket + 1] * delta;
    }

    inline float InterestPessimise(float q90Authority, float avg75Authority, float avgQuotesPerAuthor,
                            float diversity, bool isMorda, bool fast)
    {
        Y_UNUSED(isMorda);
        const static float authorityStep005[]     = {0.02f, 0.5f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f };
        const static float diversityStep005[]     = {0.07f, 0.2f, 0.75f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f };
        const static float quotesPerAuthorStep1[] = {1.0f,  1.0f, 0.9f,  0.85f, 0.82f, 0.80f, 0.78f, 0.76f, 0.75f, 0.75f};

        float res = Approximate(authorityStep005,     10, 0.05f, (q90Authority + avg75Authority) / 2.0f)
                  * Approximate(quotesPerAuthorStep1, 10, 1.0f,  avgQuotesPerAuthor);

        if (!fast) {
            res *= Approximate(diversityStep005, 10, 0.05f, diversity);
        }

        if (IsNan(res)) {
            return 1.0f;
        } else {
            return res;
        }
    }

    inline float MangoStaticPessimisation(const float* factors, bool fast)
    {
        TConstAccessor<NMango::GROUP_QUOTE_STATIC> staticFactors(factors);
        TConstAccessor<NMango::GROUP_OTHER_STATIC> otherStaticFactors(factors);

        return InterestPessimise(staticFactors[AUTHOR_AUTHORITY_Q90], staticFactors[AUTHOR_AUTHORITY_AVG75],
                                 staticFactors[LINKS_COUNT] / staticFactors[AUTHOR_COUNT],
                                 staticFactors[DIVERSITY], otherStaticFactors[IS_MORDA] > 0.5f, fast);
    }

    template<TFactorGroups GrpQuoteRel, TFactorGroups GrpOther>
    inline float MangoPessimisation(const float* factors, bool fast)
    {
        TConstAccessor<GrpQuoteRel> relQuoteFactors(factors);
        TConstAccessor<GrpOther> otherFactors(factors);

        float res = InterestPessimise(fast ? relQuoteFactors[MAX_AUTHORITY] : relQuoteFactors[AUTHOR_AUTHORITY_Q90],
                                      fast ? relQuoteFactors[MAX_AUTHORITY] : relQuoteFactors[AUTHOR_AUTHORITY_AVG75],
                                      relQuoteFactors[LINKS_COUNT] / relQuoteFactors[AUTHOR_COUNT], relQuoteFactors[DIVERSITY],
                                      false, fast);

        res *= MangoStaticPessimisation(factors, fast);

        return res;
    }


    template<TFactorGroups GrpQuoteRel>
    inline float MangoSimpleTrashless(const float* factors, bool /*fast*/)
    {
        return pow(1.0f - TConstAccessor<GrpQuoteRel>(factors)[SPAM_PROBABILITY], 0.4f);
    }

    template<TFactorGroups GrpQuoteRel>
    inline float MangoSimpleTrashlessStaticBase(const float* factors, bool /*fast*/)
    {
        TConstAccessor<NMango::GROUP_OTHER_STATIC> othStaticFactors(factors);
        float res = (1.0f - TConstAccessor<GrpQuoteRel>(factors)[SPAM_PROBABILITY]);
        return pow(res, 0.4f);
    }

    #define MangoSimpleTrashlessStatic MangoSimpleTrashlessStaticBase<NMango::GROUP_QUOTE_STATIC>
    #define MangoSimpleTrashlessHost MangoSimpleTrashlessStaticBase<NMango::GROUP_QUOTE_HOST>

    template<TFactorGroups GrpRelevance, TRelevanceFactorsLine RelFact>
    inline float MangoSimpleRelevance(const float* factors, bool /*fast*/)
    {
        TConstAccessor<GrpRelevance> relFactors(factors);
        float res = (
              relFactors[RelFact]
            + relFactors[PHRASE_TR]
        ) / 2.0f;

        return res;
    }

    inline float CalcDiversityInfluence(float diversity, float quoteCount)
    {
        if (diversity <= 0.0f) {
            return 1.0f; // чтобы разнообразие не занулило статранк при условии отсутствия морфологии
        }
        if (diversity < 0.9f) {
            return diversity;
        }
        return 0.9f * exp(-2.0f * (quoteCount - 1.0f) * (diversity - 0.9f));
    }

    template<TFactorGroups GrpQuoteRel, TFactorGroups GrpOther>
    inline float MangoSimpleInterestingness(const float* factors, bool fast)
    {
        TConstAccessor<GrpQuoteRel> relQuoteFactors(factors);
        TConstAccessor<GrpOther> otherFactors(factors);

        float res = (log(1.0f + otherFactors[DECAYED_AUTHOR_AUTHORITY_SUM]))
                  * (pow(otherFactors[AUTHOR_TOPIC_SHARE], 0.35f));

        if (!fast) {
            res *= CalcDiversityInfluence(relQuoteFactors[DIVERSITY], relQuoteFactors[LINKS_COUNT]);
        }

        return pow(res, 0.5f);
    }

    inline float MangoSimpleInterestingnessStatic(const float* factors, bool /*fast*/)
    {
        TConstAccessor<NMango::GROUP_QUOTE_STATIC> staticFactors(factors);
        TConstAccessor<NMango::GROUP_OTHER_STATIC> otherStaticFactors(factors);

        float authoritySum = sqrt(Max(0.0f, staticFactors[AUTHOR_AUTHORITY_SUM]));
        authoritySum += sqrt(Max(0.0f, otherStaticFactors[LIKE_AUTHORITY_SUM]));

        float res = (pow(CalcDiversityInfluence(staticFactors[DIVERSITY], staticFactors[LINKS_COUNT]), 0.5f)) * log(1.0f + authoritySum);

        return res;
    }

    inline float MangoSimpleInterestingnessHost(const float* factors, bool /*fast*/)
    {
        TConstAccessor<NMango::GROUP_QUOTE_HOST> hostFactors(factors);

        float res = log(hostFactors[AUTHOR_AUTHORITY_SUM] + 1.0f);

        return res;
    }

    inline float MangoSimpleFormula(const float* factors)
    {
        return TConstAccessor<GROUP_W0_FINAL>(factors)[SIMPLE_FORMULA_1];
    }

} // NMango


#include <kernel/lingboost/freq.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NLingBoost;

Y_UNIT_TEST_SUITE(FreqTest) {
    Y_UNIT_TEST(TestInvalid) {
        UNIT_ASSERT(!IsValidFreq(InvalidFreq));
        UNIT_ASSERT(!IsValidRevFreq(InvalidRevFreq));

        UNIT_ASSERT(InvalidFreq == RevFreqToFreq(InvalidRevFreq));
        UNIT_ASSERT(InvalidRevFreq == FreqToRevFreq(InvalidFreq));
    }

    Y_UNIT_TEST(TestPosFreq) {
        UNIT_ASSERT(IsValidFreq(RevFreqToFreq(Max<i64>())));
        UNIT_ASSERT(RevFreqToFreq(Max<i64>()) > 0.0f);
    }

    Y_UNIT_TEST(TestMonotonicity) {
        i64 prev = Max<i64>();
        Cerr << "staring 0\t" << prev << Endl;
        for (int exp : xrange(-20, 0)) {
           float freq = pow(10.0f, exp);
           Cerr << "freq->rev-freq" << freq << "\t" << FreqToRevFreq(freq) << Endl;
           UNIT_ASSERT(freq > 0.0f);
           UNIT_ASSERT(IsValidFreq(freq));
           UNIT_ASSERT(IsValidRevFreq(FreqToRevFreq(freq)));
           UNIT_ASSERT(FreqToRevFreq(freq) <= prev);
           prev = FreqToRevFreq(freq);
        }

        float prev2 = 1.0f;
        for (int exp : xrange(1, 60)) {
            i64 revFreq = i64(1) << exp;
            UNIT_ASSERT(IsValidRevFreq(revFreq));
            UNIT_ASSERT(IsValidFreq(RevFreqToFreq(revFreq)));
            UNIT_ASSERT(RevFreqToFreq(revFreq) <= prev2);
            prev2 = RevFreqToFreq(revFreq);
        }
    }

    Y_UNIT_TEST(TestClip) {
        UNIT_ASSERT(IsValidFreq(ClipFreq(0.0f)));
        UNIT_ASSERT(IsValidFreq(ClipFreq(-1.0f)));
        UNIT_ASSERT(IsValidFreq(ClipFreq(2.0f)));
        UNIT_ASSERT(IsValidRevFreq(ClipRevFreq(-1)));
    }

    Y_UNIT_TEST(TestCutoffValues) {
        UNIT_ASSERT(IsValidRevFreq(RevFreqCutoff));
        UNIT_ASSERT(IsValidFreq(FreqCutoff));
    }
}

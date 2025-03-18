#include "prob_counter.h"

#include <library/cpp/testing/unittest/registar.h>
#include <util/random/mersenne.h>

TMersenne<ui32> rnd;

unsigned int RandImpl() {
    return rnd.GenRand();
}

void RandReset() {
    rnd = TMersenne<ui32>();
}

Y_UNIT_TEST_SUITE(TProbCounterTest) {
    Y_UNIT_TEST(TestCounter) {
        RandReset();
        TFMCounter32 cnt;
        float averr = 0, errdisp = 0;
        for (int i = 1; i <= 1000; ++i) {
            TString t;
            for (int j = 0; j < 100; ++j) {
                t.push_back('a' + RandImpl() % 26);
            }

            TFMCounter32 tcnt;
            tcnt.Add(t.data());

            cnt.Merge(tcnt);

            float err = i / cnt.Count();
            if (err < 1.0) {
                err = 1.0 / err;
            }
            errdisp += err * err;
            averr += err;

            TString buf;
            cnt.Save(buf);
            tcnt.Load(buf);
            UNIT_ASSERT_EQUAL(cnt.Count(), tcnt.Count());
        }

        averr /= 1000.0f;
        errdisp /= 1000.0f;

        UNIT_ASSERT(averr <= 1.3f);
        UNIT_ASSERT(errdisp <= 2.0f);
    }

    Y_UNIT_TEST(TestSummator) {
        RandReset();
        TFMSummator32 sum;
        float averr = 0, errdisp = 0, rsumm = 0;
        for (int i = 1; i <= 1000; ++i) {
            TString t;
            for (int j = 0; j < 100; ++j) {
                t.push_back('a' + RandImpl() % 26);
            }

            float r = RandImpl() % 32768 / 100.0f + 1e-5f;
            rsumm += r;
            TFMSummator32 tsum;
            tsum.Add(t.data(), r);

            sum.Merge(tsum);

            float err = rsumm / sum.Sum();
            if (err < 1.0) {
                err = 1.0 / err;
            }
            errdisp += err * err;
            averr += err;

            TString buf;
            sum.Save(buf);
            tsum.Load(buf);
            UNIT_ASSERT_EQUAL(sum.Sum(), tsum.Sum());
        }

        averr /= 1000.0f;
        errdisp /= 1000.0f;

        UNIT_ASSERT(averr <= 1.5f);
        UNIT_ASSERT(errdisp <= 2.0f);
    }

    Y_UNIT_TEST(TestQuantile) {
        RandReset();
        TFMSummator32 sum(1e6, 1e-6);
        for (int i = 1; i <= 1000; ++i) {
            TString t;
            for (int j = 0; j < 110; ++j) {
                t.push_back('a' + RandImpl() % 26);
            }
            sum.Add(t.data(), i);
        }

        for (int i = 1; i < 1000; ++i) {
            float p = i / 1000.0f;
            float q = sum.CalcQuantile(p);
            UNIT_ASSERT(q >= i / 2.0f);
            UNIT_ASSERT(q <= i * 2.0f);
        }

        UNIT_ASSERT_EQUAL(sum.CalcQuantile(0.0f), 1e-6f);
        UNIT_ASSERT_EQUAL(sum.CalcQuantile(-1.0f), 1e-6f);
        UNIT_ASSERT_EQUAL(sum.CalcQuantile(1.0f), 1e6f);
        UNIT_ASSERT_EQUAL(sum.CalcQuantile(2.0f), 1e6f);
    }

    Y_UNIT_TEST(TestAvgValueInInterval) {
        RandReset();
        TFMSummator32 sum(1e6, 1e-6);
        for (int i = 1; i <= 1000; ++i) {
            TString t;
            for (int j = 0; j < 100; ++j) {
                t.push_back('a' + RandImpl() % 26);
            }
            sum.Add(t.data(), i);
        }

        for (int i = 1; i < 1000; ++i) {
            int l = RandImpl() % 1000, r = RandImpl() % 1000;
            if (l > r) {
                std::swap(l, r);
            }
            if (l > r) {
                float res = sum.CalcAvgValueInInterval(l, r);
                UNIT_ASSERT(res >= (l + r) / 4.0f);
                UNIT_ASSERT(res <= (l + r) / 1.0f);
            }
        }

        UNIT_ASSERT_EQUAL(sum.CalcAvgValueInInterval(800.0f, 100.0f), 0.0f);
    }
}

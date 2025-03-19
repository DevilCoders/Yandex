#include <kernel/text_machine/parts/common/floats.h>

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>
#include <util/random/random.h>

#include <iomanip>

using namespace NTextMachine;
using namespace NCore;

class TSeqOpsTest : public TTestBase {
private:
    UNIT_TEST_SUITE(TSeqOpsTest);
        UNIT_TEST(TestFloatCollection);
        UNIT_TEST(TestFloatRefCollection);
    UNIT_TEST_SUITE_END();

private:
    template <typename BufType1, typename BufType2>
    void CheckValueSeq(BufType1& buf1, BufType2& buf2, size_t size) {
        float x1 = size > 0 ? buf1[0] : 0.0;
        float x2 = size > 0 ? buf2[0] : 0.0;

        Cdbg << "ACCUM " << size << " = " << NSeq4f::Accum(0.0f, buf1.AsSeq4f()) << Endl;
        UNIT_ASSERT_DOUBLES_EQUAL(x1 * float(size), NSeq4f::Accum(0.0f, buf1.AsSeq4f()), 1e-8f);

        auto seq1 = buf1.AsSeq4f();
        auto seq2 = buf2.AsSeq4f();
        UNIT_ASSERT_EQUAL(size, seq1.Avail());
        UNIT_ASSERT_EQUAL(size, seq2.Avail());
        Cdbg << "ADD " << size << Endl;
        buf1.CopyFrom(NSeq4f::Add(std::move(seq1), std::move(seq2)));
        for (size_t i : xrange(size)) {
            Cdbg << "\tBUF1[" << i << "] = " << buf1[i] << Endl;
            UNIT_ASSERT_EQUAL(x1 + x2, buf1[i]);
        }

        seq1 = buf1.AsSeq4f();
        seq2 = buf2.AsSeq4f();
        UNIT_ASSERT_EQUAL(size, seq1.Avail());
        UNIT_ASSERT_EQUAL(size, seq2.Avail());
        Cdbg << "COPY " << size << Endl;
        buf1.CopyFrom(std::move(seq2));
        for (size_t i : xrange(size)) {
            Cdbg << "\tBUF1[" << i << "] = " << buf1[i] << Endl;
            UNIT_ASSERT_EQUAL(x2, buf1[i]);
        }

        seq1 = buf1.AsSeq4f();
        seq2 = buf2.AsSeq4f();
        UNIT_ASSERT_EQUAL(size, seq1.Avail());
        UNIT_ASSERT_EQUAL(size, seq2.Avail());
        Cdbg << "ADDMUL " << size << Endl;
        buf1.CopyFrom(NSeq4f::Add(std::move(seq1), std::move(seq2), 3.0));
        for (size_t i : xrange(size)) {
            Cdbg << "\tBUF1[" << i << "] = " << buf1[i] << Endl;
            UNIT_ASSERT_EQUAL(4.0f * x2, buf1[i]);
        }

        seq1 = buf1.AsSeq4f();
        seq2 = buf2.AsSeq4f();
        UNIT_ASSERT_EQUAL(size, seq1.Avail());
        UNIT_ASSERT_EQUAL(size, seq2.Avail());
        buf1.CopyFrom(NSeq4f::Add(std::move(seq1), NSeq4f::Log(std::move(seq2), 1.0)));
        Cdbg << "ADDLOG " << size << Endl;
        for (size_t i : xrange(size)) {
            Cdbg << "\tBUF1[" << i << "] = " << buf1[i] << Endl;
            UNIT_ASSERT_DOUBLES_EQUAL(4.0f * x2 + 1.0f * float(log(1.0f + x2)), buf1[i], 1e-8f);
        }

        buf1.CopyFrom(buf2.AsSeq4f());
        seq1 = buf1.AsSeq4f();
        seq2 = buf2.AsSeq4f();
        UNIT_ASSERT_EQUAL(size, seq1.Avail());
        UNIT_ASSERT_EQUAL(size, seq2.Avail());
        buf1.CopyFrom(NSeq4f::Add(std::move(seq1), NSeq4f::Norm(std::move(seq2), 2.0), 1.0));
        Cdbg << "ADDNORM " << size << Endl;
        for (size_t i : xrange(size)) {
            Cdbg << "\tBUF1[" << i << "] = " << buf1[i] << Endl;
            UNIT_ASSERT_DOUBLES_EQUAL(x2 + 1.0f * (x2 / (x2 + 2.0f)), buf1[i], 1e-8f);
        }

        float res = 1.0f;
        for (size_t n : xrange(10)) {
            seq2 = buf2.AsSeq4f();
            UNIT_ASSERT_EQUAL(size, seq2.Avail());
            buf1.CopyFrom(NSeq4f::Pow(std::move(seq2), n));
            Cdbg << "IPOW(" << n << ") " << size << Endl;
            for (size_t i : xrange(size)) {
                Cdbg << "\tBUF1[" << i << "] = " << buf1[i] << Endl;
                UNIT_ASSERT_DOUBLES_EQUAL(res, buf1[i], 1e-8f);
            }
            res *= x2;
        }
    }

public:
    void TestFloatCollection() {
        TMemoryPool pool(10 << 10);
        TFloatCollection buf1;
        TFloatCollection buf2;

        for (size_t size : xrange(32)) {
            buf1.Init(pool, size);
            buf2.Init(pool, size);

            for (size_t i : xrange(size)) {
                buf1[i] = 1.0;
            }
            for (size_t i : xrange(size)) {
                buf2[i] = 2.0;
            }

            CheckValueSeq(buf1, buf2, size);
        }
    }

    void TestFloatRefCollection() {
        TMemoryPool pool(10 << 10);
        TFloatCollection buf1;
        TFloatCollection buf2;
        TFloatRefCollection ref2;

        for (size_t size : xrange(32)) {
            buf1.Init(pool, size);
            buf2.Init(pool, size);
            ref2.Init(pool, size);

            for (size_t i : xrange(size)) {
                buf1[i] = 1.0;
            }
            for (size_t i : xrange(size)) {
                buf2[i] = 2.0;
            }
            for (size_t i : xrange(size)) {
                ref2.Bind(i, &buf2[i]);
            }

            CheckValueSeq(buf1, ref2, size);
        }
    }

    void TestFloatCollectionAcc() {
        TMemoryPool pool(10 << 10);
        TFloatCollection buf1;
        TFloatCollection buf2;

        for (size_t size : xrange(32)) {
            buf1.Init(pool, size);
            buf2.Init(pool, size);

            for (size_t i : xrange(size)) {
                buf1[i] = 1.0;
            }
            for (size_t i : xrange(size)) {
                buf2[i] = 2.0;
            }

            CheckValueSeq(buf1, buf2, size);
        }
    }

};

UNIT_TEST_SUITE_REGISTRATION(TSeqOpsTest);

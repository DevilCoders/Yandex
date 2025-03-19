#include <kernel/proto_fuzz/fuzzy_output.h>

#include <util/generic/buffer.h>
#include <util/stream/buffer.h>
#include <util/stream/format.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/xrange.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NProtoFuzz;

const ui64 seed = 11;

Y_UNIT_TEST_SUITE(FuzzyOutputTest) {
    Y_UNIT_TEST(TestZero) {
        TBufferOutput buf;
        TFuzzyOutput out{buf, TFuzzyOutput::TOptions(seed)};

        out.Write((const void*)nullptr, 0);
        out.Finish();

        Cdbg << "BUF_SIZE = (" << buf.Buffer().Size() << ")" << Endl;
        UNIT_ASSERT_EQUAL(buf.Buffer().Size(), 0);
    }

    Y_UNIT_TEST(TestPartSizeEq1) {
        TBufferOutput buf;

        TFuzzyOutput::TOptions opts(seed);
        opts.PartSize = 1;
        TFuzzyOutput out{buf, opts};

        const TVector<char> zeroes(100);

        out.Write(zeroes.data(), 1);
        out.Flush();

        Cdbg << "BUF_SIZE_1 = (" << buf.Buffer().Size() << ")" << Endl;
        UNIT_ASSERT_EQUAL(buf.Buffer().Size(), 1);
        Cdbg << "BUF_DATA_1 = (" << Hex(buf.Buffer().Data()[0]) << ")" << Endl;
        UNIT_ASSERT_UNEQUAL(buf.Buffer().Data()[0], 0);

        buf.Buffer().Clear();
        out.Write(zeroes.data(), zeroes.size());

        Cdbg << "BUF_SIZE_2 = (" << buf.Buffer().Size() << ")" << Endl;
        UNIT_ASSERT_EQUAL(buf.Buffer().Size(), zeroes.size());
        Cdbg << "BUF_DATA_2 = (";
        for (size_t i : xrange(zeroes.size())) {
            Cdbg << Hex(buf.Buffer().Data()[i]) << " ";
            UNIT_ASSERT_UNEQUAL(buf.Buffer().Data()[i], 0);
        }
        Cdbg << ")" << Endl;
    }

    Y_UNIT_TEST(TestPartSizeEq10) {
        TBufferOutput buf;

        TFuzzyOutput::TOptions opts(seed);
        opts.PartSize = 10;
        TFuzzyOutput out{buf, opts};

        const TVector<char> zeroes(100);
        out.Write(zeroes.data(), zeroes.size());

        Cdbg << "BUF_SIZE = (" << buf.Buffer().Size() << ")" << Endl;
        UNIT_ASSERT_EQUAL(buf.Buffer().Size(), zeroes.size());
        Cdbg << "BUF_DATA = (";
        for (size_t i = 0; i < zeroes.size(); i += 10) {
            size_t count = 0;
            for (size_t j = 0; j < 10; ++j) {
                Cdbg << Hex(buf.Buffer().Data()[i + j]) << " ";
                if (buf.Buffer().Data()[i + j] != 0) {
                    count += 1;
                }
            }
            if (i + 10 < zeroes.size()) {
                Cdbg << "| ";
            }
            UNIT_ASSERT_EQUAL(count, 1);
        }
        Cdbg << ")" << Endl;
    }

    Y_UNIT_TEST(TestPartSizeEq10Partial) {
        TBufferOutput buf;
        TFuzzyOutput::TOptions opts;
        opts.PartSize = 10;

        TFastRng64 rng(seed);

        const size_t samples = 10000;

        const TVector<char> zeroes(10);
        TVector<size_t> counts(10);

        for (size_t size : xrange(10)) {
            for (size_t i : xrange(samples)) {
                Y_UNUSED(i);

                buf.Buffer().Clear();
                opts.Seed = rng.GenRand64();
                TFuzzyOutput out{buf, opts};
                out.Write(zeroes.data(), size);

                UNIT_ASSERT_EQUAL(buf.Buffer().Size(), size);

                const char* ptr = FindIf(buf.Buffer().Begin(), buf.Buffer().End(),
                    [](char x){ return x != 0; });

                if (ptr != buf.Buffer().End()) {
                    counts[size] += 1;
                }
            }
        }

        Cdbg << "FRACS = (";
        for (size_t i : xrange(10)) {
            const double frac = double(counts[i]) / double(samples);
            const double err = fabs(frac - 0.1 * i) / Max(1e-6, 0.1 * i);
            Cdbg << frac << " {err=" << err << "} " ;
            UNIT_ASSERT(err >= 0.0 && err < 0.1); // if it fails, try to increase samples
        }
        Cdbg << ")" << Endl;
    }

    Y_UNIT_TEST(TestPartSizeEq1Values) {
        TBufferOutput buf;
        TFuzzyOutput::TOptions opts;
        opts.PartSize = 1;

        TFastRng64 rng(seed);

        const size_t samples = 255 * 1000;

        const TVector<char> zeroes(1);
        TVector<size_t> counts(255);

        for (size_t i : xrange(samples)) {
            Y_UNUSED(i);

            buf.Buffer().Clear();
            opts.Seed = rng.GenRand64();
            TFuzzyOutput out{buf, opts};
            out.Write(zeroes.data(), 1);

            UNIT_ASSERT_EQUAL(buf.Buffer().Size(), 1);

            const unsigned char value = buf.Buffer().Data()[0];
            UNIT_ASSERT_UNEQUAL(value, 0);

            counts[value - 1] += 1;
        }

        Cdbg << "FRACS = (";
        for (size_t i : xrange(255)) {
            const double frac = double(counts[i]) / double(samples);
            const double err = fabs(frac - 1.0 / 255.0) * 255.0;
            Cdbg << frac << " {err=" << err << "} " ;
            UNIT_ASSERT(err >= 0.0 && err < 0.1); // if it fails, try to increase samples
        }
        Cdbg << ")" << Endl;
    }
}

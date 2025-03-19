#include <kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/optimized_multiply_add.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/xrange.h>
#include <util/generic/deque.h>
#include <util/random/fast.h>

using namespace NNeuralNetApplier;

using TRandomGetter = TReallyFastRng32;

struct TPackedData {
    TVector<ui8> Data;
    ui32 Dimension = 0;
    ui32 Rows = 0;

    float Min = 0;
    float Coeff = 0;

    void Init(TRandomGetter& rg, ui32 dim, ui32 rows) {
        Data.reserve(dim * rows);
        Dimension = dim;
        Rows = rows;
        for([[maybe_unused]] auto i : xrange(dim * rows)) {
            Data.push_back(rg.Uniform(Max<ui8>()));
        }
        Min = rg.GenRandReal1();
        Coeff = rg.GenRandReal1();
    }
};

struct TCase {
    const TPackedData* PackedData = nullptr;
    TVector<size_t> Rows;
    TVector<float> Weights;
    TVector<float> Result;

    void Init(TRandomGetter& rg, ui32 rowsNum) {
        for([[maybe_unused]] auto i : xrange(rowsNum)) {
            Rows.push_back(rg.GenRandReal1() * PackedData->Rows);
            Weights.push_back(rg.GenRandReal1());
        }
        Result.resize(PackedData->Dimension);
        NNeuralNetApplier::DoMultiplyAddWithUnpackBatchedSimple(
            PackedData->Dimension,
            PackedData->Data,
            Result,
            Weights,
            Rows,
            PackedData->Coeff,
            PackedData->Min
        );
    }
};

struct TTester {
    TReallyFastRng32 RNG = {29}; //just some fixed number
    TDeque<TPackedData> PackedDataVariants;
    TDeque<TCase> Cases;

    TTester() {
        PackedDataVariants.emplace_back().Init(RNG, 10, 1000);
        PackedDataVariants.emplace_back().Init(RNG, 10, 10);
        PackedDataVariants.emplace_back().Init(RNG, 10, 100000);
        PackedDataVariants.emplace_back().Init(RNG, 32, 1000);
        PackedDataVariants.emplace_back().Init(RNG, 32, 10);
        PackedDataVariants.emplace_back().Init(RNG, 32, 100000);
        PackedDataVariants.emplace_back().Init(RNG, 51, 1000);
        PackedDataVariants.emplace_back().Init(RNG, 51, 10);
        PackedDataVariants.emplace_back().Init(RNG, 51, 100000);
        for(auto& p : PackedDataVariants) {
            for(auto i : xrange(10)) {
                auto& c = Cases.emplace_back();
                c.PackedData = &p;
                c.Init(RNG, i);
            }
        }
        for(auto d : xrange(1, 256)) {
            PackedDataVariants.emplace_back().Init(RNG, d, 1000);
            auto& c = Cases.emplace_back();
            c.PackedData = &PackedDataVariants.back();
            c.Init(RNG, 10);
        }
    }

    template<class Func>
    void Run(const Func& f) const {
        for(auto& c : Cases) {
            TVector<float> res(c.PackedData->Dimension);
            f(
                c.PackedData->Dimension,
                c.PackedData->Data,
                res,
                c.Weights,
                c.Rows,
                c.PackedData->Coeff,
                c.PackedData->Min
            );
            for(auto i : xrange(c.PackedData->Dimension)) {
                UNIT_ASSERT_VALUES_EQUAL(res[i] - c.Result[i], 0);
            }
        }
    }
    template<class Func>
    void RunNotExact(const Func& f) const {
        for(auto& c : Cases) {
            TVector<float> res(c.PackedData->Dimension);
            f(
                c.PackedData->Dimension,
                c.PackedData->Data,
                res,
                c.Weights,
                c.Rows,
                c.PackedData->Coeff,
                c.PackedData->Min
            );
            for(auto i : xrange(c.PackedData->Dimension)) {
                UNIT_ASSERT_DOUBLES_EQUAL(res[i] - c.Result[i], 0, 1e-3);
            }
        }
    }
};

Y_UNIT_TEST_SUITE(TestMultiplyAdd) {
    TTester Tester;
    Y_UNIT_TEST(Simple) {
        Tester.Run(DoMultiplyAddWithUnpackBatchedSimple);
    }
    Y_UNIT_TEST(Auto) {
        Tester.Run(DoMultiplyAddWithUnpackBatched);
    }
    Y_UNIT_TEST(Avx) {
        if (NX86::CachedHaveAVX2()) {
            Tester.Run(DoMultiplyAddWithUnpackBatchedAVX2);
        }
    }
    Y_UNIT_TEST(Sse) {
        if (NX86::CachedHaveSSE41()) {
            Tester.Run(DoMultiplyAddWithUnpackBatchedSSE41);
        }
    }
}


#include <kernel/dssm_applier/nn_applier/lib/optimized_multiply_add/optimized_multiply_add.h>
#include <library/cpp/testing/benchmark/bench.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/xrange.h>
#include <util/generic/deque.h>
#include <util/random/fast.h>

#ifdef _sse_
#include <library/cpp/vec4/vec4.h>
#endif

using namespace NNeuralNetApplier;

using TRandomGetter = TReallyFastRng32;

struct TPackedData {
    TVector<ui8> Data;
    ui32 Dimension = 0;
    ui32 Rows = 0;

    float Min = 0;
    float Coeff = 0;
    TVector<float> DictForOldFashion;

    TPackedData(TRandomGetter& rg, ui32 dim, ui32 rows) {
        for([[maybe_unused]] auto i : xrange(256)) {
            DictForOldFashion.push_back(rg.GenRandReal1());
        }
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

    void Init(TPackedData& pd, TRandomGetter& rg, ui32 rowsNum) {
        PackedData = &pd;
        for([[maybe_unused]] auto i : xrange(rowsNum)) {
            Rows.push_back(rg.GenRandReal1() * PackedData->Rows);
            Weights.push_back(rg.GenRandReal1());
        }
        Result.resize(PackedData->Dimension);
    }
};

struct TCasesRunner {
    TVector<TCase> Cases;
    TVector<float> ResultBuffer;

    TCasesRunner(TPackedData& pd, TRandomGetter& rg, ui32 rowsNumMax) {
        ResultBuffer.resize(pd.Dimension);
        for([[maybe_unused]] auto i : xrange(1000)) {
            Cases.emplace_back().Init(pd, rg, rg.GenRandReal1() * rowsNumMax + 1);
        }
    }

    template<class Func>
    void Run(const Func& f, size_t runsNum) {
        for(auto i : xrange(runsNum)) {
            auto& c = Cases[i % Cases.size()];
            f(
                c.PackedData->Dimension,
                c.PackedData->Data,
                ResultBuffer,
                c.Weights,
                c.Rows,
                c.PackedData->Coeff,
                c.PackedData->Min
            );
        }
    }

    static Y_NO_INLINE void OldFachionedCalcMutliplyAndAddRowTo(const float* dict, const ui8* indices, size_t nCols, float factor, float* output) {
        size_t i = 0;
#ifdef _sse_
        if (nCols >= 4) {
            TVec4f factorVec(factor);
            for (; i + 4 <= nCols; i += 4) {
                TVec4f rowVec(
                    dict[indices[i]],
                    dict[indices[i + 1]],
                    dict[indices[i + 2]],
                    dict[indices[i + 3]]
                );
                rowVec *= factorVec;
                TVec4f outVec(output + i);
                outVec += rowVec;
                outVec.Store(output + i);
            }
        }
#endif
        for (; i < nCols; ++i) {
            output[i] += dict[indices[i]] * factor;
        }
    }

    void RunOldFashion(size_t runsNum) {
        for(auto i : xrange(runsNum)) {
            auto& c = Cases[i % Cases.size()];
            ResultBuffer.assign(ResultBuffer.size(), 0.f);
            for(auto k : xrange(c.Weights.size())) {
                OldFachionedCalcMutliplyAndAddRowTo(
                    c.PackedData->DictForOldFashion.data(),
                    c.PackedData->Data.begin() + c.PackedData->Dimension * c.Rows[k],
                    c.PackedData->Dimension,
                    c.Weights[k],
                    ResultBuffer.begin()
                );
            }
        }
    }
};

struct TTestData {
    TReallyFastRng32 Rng = {32};
    TPackedData Dim40Rows1K = {Rng, 40, 1000};
    TPackedData Dim40Rows100K = {Rng, 40, 100 * 1000};

    TPackedData Dim80Rows100K = {Rng, 80, 100 * 1000};
    TPackedData Dim120Rows100K = {Rng, 120, 100 * 1000};
    TPackedData Dim150Rows100K = {Rng, 150, 100 * 1000};
    TPackedData Dim210Rows100K = {Rng, 210, 100 * 1000};

    TCasesRunner Dim40Rows1K_CasesMax40 = {Dim40Rows1K, Rng, 40};
    TCasesRunner Dim40Rows100K_CasesMax40 = {Dim40Rows100K, Rng, 40};

    TCasesRunner Dim80Rows100K_CasesMax40 = {Dim80Rows100K, Rng, 40};
    TCasesRunner Dim120Rows100K_CasesMax40 = {Dim120Rows100K, Rng, 40};
    TCasesRunner Dim150Rows100K_CasesMax40 = {Dim150Rows100K, Rng, 40};
    TCasesRunner Dim210Rows100K_CasesMax40 = {Dim210Rows100K, Rng, 40};

    TCasesRunner Dim80Rows100K_CasesMax10 = {Dim80Rows100K, Rng, 10};
    TCasesRunner Dim120Rows100K_CasesMax10 = {Dim120Rows100K, Rng, 10};
    TCasesRunner Dim150Rows100K_CasesMax10 = {Dim150Rows100K, Rng, 10};
    TCasesRunner Dim210Rows100K_CasesMax10 = {Dim210Rows100K, Rng, 10};

    TTestData() {
        Cerr << "Init finished" << Endl;
    }
} TestData;

//

Y_CPU_BENCHMARK(Dim40Rows1K_CasesMax40_Old, iface) {
    TestData.Dim40Rows1K_CasesMax40.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows1K_CasesMax40_Simple, iface) {
    TestData.Dim40Rows1K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows1K_CasesMax40_Sse, iface) {
    TestData.Dim40Rows1K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows1K_CasesMax40_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim40Rows1K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim40, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows1K_CasesMax40_Avx, iface) {
    TestData.Dim40Rows1K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows1K_CasesMax40_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim40Rows1K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim40, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim40Rows100K_CasesMax40_Old, iface) {
    TestData.Dim40Rows100K_CasesMax40.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows100K_CasesMax40_Simple, iface) {
    TestData.Dim40Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows100K_CasesMax40_Sse, iface) {
    TestData.Dim40Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows100K_CasesMax40_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim40Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim40, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows100K_CasesMax40_Avx, iface) {
    TestData.Dim40Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim40Rows100K_CasesMax40_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim40Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim40, iface.Iterations());
}

//

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax40_Old, iface) {
    TestData.Dim80Rows100K_CasesMax40.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax40_Simple, iface) {
    TestData.Dim80Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax40_Sse, iface) {
    TestData.Dim80Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax40_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim80Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim80, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax40_Avx, iface) {
    TestData.Dim80Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax40_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim80Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim80, iface.Iterations());
}

//

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax40_Old, iface) {
    TestData.Dim120Rows100K_CasesMax40.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax40_Simple, iface) {
    TestData.Dim120Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax40_Sse, iface) {
    TestData.Dim120Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax40_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim120Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim120, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax40_Avx, iface) {
    TestData.Dim120Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax40_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim120Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim120, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax40_Old, iface) {
    TestData.Dim150Rows100K_CasesMax40.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax40_Simple, iface) {
    TestData.Dim150Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax40_Sse, iface) {
    TestData.Dim150Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax40_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim150Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim150, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax40_Avx, iface) {
    TestData.Dim150Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax40_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim150Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim150, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax40_Old, iface) {
    TestData.Dim210Rows100K_CasesMax40.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax40_Simple, iface) {
    TestData.Dim210Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax40_Sse, iface) {
    TestData.Dim210Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax40_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim210Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim210, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax40_Avx, iface) {
    TestData.Dim210Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax40_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim210Rows100K_CasesMax40.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim210, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax10_Old, iface) {
    TestData.Dim80Rows100K_CasesMax10.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax10_Simple, iface) {
    TestData.Dim80Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax10_Sse, iface) {
    TestData.Dim80Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax10_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim80Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim80, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax10_Avx, iface) {
    TestData.Dim80Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim80Rows100K_CasesMax10_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim80Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim80, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax10_Old, iface) {
    TestData.Dim120Rows100K_CasesMax10.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax10_Simple, iface) {
    TestData.Dim120Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax10_Sse, iface) {
    TestData.Dim120Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax10_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim120Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim120, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax10_Avx, iface) {
    TestData.Dim120Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim120Rows100K_CasesMax10_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim120Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim120, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax10_Old, iface) {
    TestData.Dim150Rows100K_CasesMax10.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax10_Simple, iface) {
    TestData.Dim150Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax10_Sse, iface) {
    TestData.Dim150Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax10_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim150Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim150, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax10_Avx, iface) {
    TestData.Dim150Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim150Rows100K_CasesMax10_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim150Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim150, iface.Iterations());
}

//
Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax10_Old, iface) {
    TestData.Dim210Rows100K_CasesMax10.RunOldFashion(iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax10_Simple, iface) {
    TestData.Dim210Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSimple, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax10_Sse, iface) {
    TestData.Dim210Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax10_Sse_CompileTimeKnownDim, iface) {
    TestData.Dim210Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedSSE41_Dim210, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax10_Avx, iface) {
    TestData.Dim210Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2, iface.Iterations());
}

Y_CPU_BENCHMARK(Dim210Rows100K_CasesMax10_Avx_CompileTimeKnownDim, iface) {
    TestData.Dim210Rows100K_CasesMax10.Run(DoMultiplyAddWithUnpackBatchedAVX2_Dim210, iface.Iterations());
}

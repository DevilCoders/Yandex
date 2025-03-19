#include <kernel/dssm_applier/optimized_model/test_utils/test_utils.h>

#include <library/cpp/testing/benchmark/bench.h>

namespace {
    NOptimizedModelTest::TTestEnvironment TestEnv;
}

Y_CPU_BENCHMARK(OptimizedModelApply, p) {
    for (size_t i = 0; i < p.Iterations(); ++i) {
        TestEnv.RunApplyTest([](const auto& applyResult) {
            Y_DO_NOT_OPTIMIZE_AWAY(applyResult);
            ::NBench::Clobber();
        });
    }
}

Y_CPU_BENCHMARK(OptimizedModelApplyAll, p) {
    for (size_t i = 0; i < p.Iterations(); ++i) {
        TestEnv.RunApplyAllTest([](const auto& applyAllResult) {
            Y_DO_NOT_OPTIMIZE_AWAY(applyAllResult);
            ::NBench::Clobber();
        });
    }
}

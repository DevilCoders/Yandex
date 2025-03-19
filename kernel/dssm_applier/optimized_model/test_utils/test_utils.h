#pragma once

#include <kernel/dssm_applier/optimized_model/test_utils/optimized_model_params.pb.h>

#include <kernel/dssm_applier/optimized_model/optimized_model_applier.h>

namespace NOptimizedModelTest {

    class TTestEnvironment {
        enum class EType {
            Apply,
            ApplyAll,
        };

    public:

        TTestEnvironment();
        ~TTestEnvironment();

        template <typename TCallback>
        void RunApplyTest(TCallback&& callback) const {
            return RunTest<EType::Apply>(std::forward<TCallback>(callback));
        }

        template <typename TCallback>
        void RunApplyAllTest(TCallback&& callback) const {
            return RunTest<EType::ApplyAll>(std::forward<TCallback>(callback));
        }

    private:

        template <EType type, typename TCallback>
        void RunTest(TCallback&& callback) const {
            if (!OptimizedModel) {
                return;
            }
            for (auto& frame : InputDump->GetFrames()) {
                NOptimizedModel::TVariablesInput input;

                for (auto& header : frame.GetHeaders()) {
                    input.Headers.emplace(header.GetKey(), header.GetValue());
                }
                for (auto& extInput : frame.GetExternalInputs()) {
                    auto& inputValues = input.ExternalInputs[extInput.GetKey()];
                    inputValues.reserve(extInput.ValuesSize());
                    inputValues.assign(extInput.GetValues().begin(), extInput.GetValues().end());
                }
                if constexpr (type == EType::Apply) {
                    NOptimizedModel::TApplyConfig config;
                    config.FlagsVersion = frame.GetWebL3ModelsFlagsVersion();
                    for (auto& rw : frame.GetWebL3ModelsVersionsRewrites()) {
                        config.ModelsVersionsRewrites.emplace(rw.GetKey(), rw.GetValue());
                    }
                    callback(OptimizedApplier->Apply(config, input));
                }
                if constexpr (type == EType::ApplyAll) {
                    NOptimizedModel::TApplyConfig applyConfig;
                    applyConfig.ApplyAll = true;
                    callback(OptimizedApplier->ApplyEx(std::move(applyConfig), input));
                }
            }
        }

    private:
        THolder<NNeuralNetApplier::TModel> OptimizedModel;
        THolder<NOptimizedModel::TInputDump> InputDump;
        THolder<NOptimizedModel::TOptimizedModelApplier> OptimizedApplier;
    };
}

#include "options.h"

#include <kernel/dssm_applier/optimized_model/test_utils/test_utils.h>

#include <kernel/dssm_applier/embeddings_transfer/embeddings_transfer.h>

#include <library/cpp/yson/node/node_io.h>

#include <util/stream/output.h>
#include <util/string/cast.h>
#include <util/system/env.h>

namespace {
    NOptimizedModelTest::TTestEnvironment TestEnv;

    auto FloatToStr(float n) {
        return FloatToString(n, PREC_POINT_DIGITS, 5);
    };

    void ApplyTest(const TVector<TString>& names) {
        TestEnv.RunApplyTest([&names](const auto& arg) {
            for (size_t i = 0; i < names.size(); ++i) {
                Cout << FloatToStr(arg.GetPredict(names[i]));
                if (i != names.size() - 1) {
                    Cout << '\t';
                }
            }
            Cout << '\n';
        });
    }

    void ApplyAllTest(const TVector<TString>& names) {
        TSet<TString> setNames(names.begin(), names.end());
        TestEnv.RunApplyAllTest([](const auto& arg) {
            Cout << NYT::NodeToYsonString(arg.DumpToYson(), NYson::EYsonFormat::Pretty) << '\n';
        });
    }

} // namespace

int main(int argc, const char* argv[]) {
    SetEnv("MKL_CBWR", "COMPATIBLE");
    NTest::TOptions options;
    options.Parse(argc, argv);

    const TVector<TString> names{
        "joint_output_long_middle_short_vs_hard_clicks",
        "joint_output_long_vs_middle_short_no_clicks",
        "joint_output_middle_vs_short_long_hard_no_clicks",
        "joint_output_short_vs_middle_long_hard_no_clicks",
        "joint_output_no_vs_short_middle_long_hard_clicks",
        "joint_output_long_vs_short_middle_hard_clicks",
        "joint_output_middle_long_vs_short_hard_clicks",
        "joint_output_short_middle_long_vs_hard_no_clicks",
        "joint_output_bigrams",
        "joint_output_one_click_probability",
        "joint_output_query_dwelltime",
        "joint_output_ctr_no_miner",
        "joint_output_log_dt_bigrams_am_hard_queries_no_clicks",
        "joint_output_log_dt_bigrams_am_hard_queries_no_clicks_mixed",
        "joint_output_ctr_eng_ss_hard",
        "joint_output_reformulations_longest_click_log_dt",
        "joint_output_reformulations_longest_click_log_dt_early_binding_dssm",
        "joint_output_reformulations_with_extensions"
    };
    if (names.empty()) {
        return 0;
    }

    Cout << names[0];
    for (size_t i = 1; i < names.size(); ++i) {
        Cout << '\t' << names[i];
    }
    Cout << '\n';

    if (options.Mode == NTest::EMode::Apply) {
        ApplyTest(names);
    }
    if (options.Mode == NTest::EMode::ApplyAll) {
        ApplyAllTest(names);
    }

    return 0;
}

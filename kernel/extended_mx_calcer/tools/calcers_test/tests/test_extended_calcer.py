# coding: utf-8
import os
import pytest
from yatest import common


STUB_CTX = '{IsDebug:1,Meta:{RandomSeed:test}}'
CONTEXT_WITH_SUB_SCORES = (
    '{"IsDebug": 1, "Meta": {"RandomSeed":test, "RawPredictions":{"AllCategScores": {'
    '"sub_win": [-2.7086,-3.1237,-3.5186,-3.9438,-4.379,-4.7480,-5.019,-5.2446,-5.35792,-5.42861],'
    '"sub_loss": [-2.93981,-3.291297,-3.941326,-4.481364,-5.09323,-5.62171,-5.9636,-6.2512,-6.459,-6.60078]}}}}'
)

test_data = [
    ['simple', 'always_2.xtd', STUB_CTX, 'factors.tsv'],
    ['multi_clickint', 'multi_clickint.xtd', STUB_CTX, 'factors.tsv'],
    [
        'clickint_random',
        'clickint_random.xtd',
        '{IsDebug:1,Meta:{RandomSeed:test, FeatureContext:{ViewType:{AvailibleValues:{"s":1, "m":1, "l":1}}}}}',
        'factors.tsv'
    ],
    ['clickint_add_random', 'clickint_add_rand.xtd', STUB_CTX, 'factors.tsv'],
    ['bad_clickint', 'bad_clickint.xtd', STUB_CTX, 'factors.tsv'],
    ['spwll', 'spwll.xtd', STUB_CTX, 'factors.tsv'],
    ['simple_meta', 'simple_meta.xtd', STUB_CTX, 'factors.tsv'],
    ['bad_meta', 'bad_meta.xtd', STUB_CTX, 'factors.tsv'],
    ['win_loss', 'win_loss.xtd', STUB_CTX, 'factors.tsv'],
    ['clickregtree', 'clickregtree.xtd', STUB_CTX, 'factors.tsv'],
    ['clickregtree_stump', 'clickregtree_stump.xtd', STUB_CTX, 'factors.tsv'],
    ['mc_with_filter', 'mc_with_filter.xtd', STUB_CTX, 'factors.tsv'],
    ['multitarget', 'multitarget.xtd', '{IsDebug:1,Meta:{RandomSeed:test, FeatureContext:{"ViewType":{"AvailibleValues":{"market_implicit_model":null,"market_model":null,"market_offers_wizard":null}}}}}', 'factors.tsv'],  # noqa: E501
    ['multitarget_fast', 'multitarget_fast.xtd', '{IsDebug:1,Meta:{RandomSeed:test, FeatureContext:{"ViewType":{"AvailibleValues":{"market_implicit_model":null,"market_model":null,"market_offers_wizard":null}}}}}', 'factors.tsv'],  # noqa: E501
    ['multitarget_with_res_feats', 'multitarget_with_res_feats.xtd', STUB_CTX, 'factors.tsv'],
    ['multibundle_by_factors', 'multibundle_by_factors.xtd', STUB_CTX, 'factors.tsv'],
    ['one_position_binary', 'one_position_binary.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_surplus', 'positional_surplus.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_multi_sam', 'multi_sam.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_softmax_with_subtargets', 'images_swifty_30bin_upper.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_perceptron', 'positional_perceptron.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_majority_vote', 'positional_bayes.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_bin_composition', 'pos_bin_composition.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_random', 'positional_random.xtd', STUB_CTX, 'factors.tsv'],
    ['positional_softmax_with_positional_subtargets', 'images_azazello_stub.xtd', STUB_CTX, 'factors.tsv'],
    ['multifeature_softmax_bin', 'video_azazello_test_bin.xtd', STUB_CTX, 'factors.tsv'],
    ['multifeature_softmax_int', 'video_azazello_test_int.xtd', STUB_CTX, 'factors.tsv'],
    ['multifeature_softmax_binint', 'video_azazello_test_binint.xtd', STUB_CTX, 'factors.tsv'],
    ['multifeature_softmax_top_binint', 'video_azazello_top_test_binint.xtd', STUB_CTX, 'factors.tsv'],
    ['view_place_pos_random', 'view_place_pos_random.xtd', '{IsDebug:1,Meta:{RandomSeed:test,AvailableVTCombinations:{"text":{"main":[0,1,2,3,4,5,6,7,8,9]},"_":{"main":[0,1,2,3,4,5,6,7,8,9],"right":[0,1,2,3,4]}}}}', 'factors.tsv'],  # noqa: E501
    ['view_place_pos_random_with_wc', 'view_place_pos_random.xtd', '{IsDebug:1,Meta:{RandomSeed:test,AvailableVTCombinations:{"text":{"main":"*"}}}}', 'factors.tsv'],
    ['view_place_pos_random_with_wc_with_noshow', 'view_place_pos_random_with_noshow.xtd', '{IsDebug:1,Meta:{RandomSeed:test,AvailableVTCombinations:{"text":{"main":"*"}}}}', 'factors.tsv'],
    ['clickint_incut_mix', 'clickint_incut_mix.xtd', STUB_CTX, 'incut_mix_factors.tsv'],
    ['random_incut_mix', 'random_incut_mix.xtd', STUB_CTX, 'incut_mix_factors.tsv'],
    ['random_mask_mix', 'random_mask_mix.xtd', STUB_CTX, 'incut_mix_factors.tsv'],
    ['binary_with_arbitary_result', 'binary_with_arbitary_result.xtd', STUB_CTX, 'factors.tsv'],
    ['weight_local_random', 'weight_local_random.xtd', '{IsDebug:1,Meta:{RandomSeed:"3", "PredictedWeightInfo": 0.21}}', 'factors.tsv'],
    ['cascade', 'cascade.xtd', STUB_CTX, 'factors.tsv'],
    ['factor_filter', 'factor_filter.xtd', STUB_CTX, 'factors.tsv'],
    ['multifeature_winloss_mc', 'multifeature_winloss_mc.xtd', STUB_CTX, 'multifeature_winloss_mc_factors.tsv'],
    ['mx_with_meta', 'mx_with_meta.xtd', STUB_CTX, 'factors.tsv'],
    ['multifeature_winloss_mc_with_relev_boost', 'winloss_mc_with_relev_boost_test_bundle.xtd', STUB_CTX, 'multifeature_winloss_mc_with_relev_boost_factors.tsv'],
    ['multifeature_winloss_mc_with_relev_boost_recalc', 'winloss_mc_with_relev_boost_test_bundle.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:all}}', 'multifeature_winloss_mc_with_relev_boost_factors.tsv', '{"RelevBoostParams": {"SurplusCoef": 123}}'],  # noqa: E501
    ['multifeature_winloss_mc_with_combination_boost', 'winloss_mc_with_combination_boost.xtd', STUB_CTX, 'multifeature_winloss_mc_with_combination_boost.tsv'],
    ['winloss_mc_with_usefeaturesascategorical', 'bundle_with_catfeatures.xtd', STUB_CTX, 'bunlde_with_catfeatures_factors.tsv'],
    ['any_with_random', 'alwyas_one_with_random.xtd', STUB_CTX, 'factors.tsv'],
    ['model_with_no_position_always_available', 'market_model_with_noshows.xtd', '{"IsDebug": True, "Meta": {"RandomSeed": "test","FeatureContext": {"ViewType": {"AvailibleValues": {"market_offers_wizard": 1}}}}}', 'model_with_no_position_always_available_data.tsv'],  # noqa: E501
    ['multifeature_winloss_mc_multipredict', 'multifeature_winloss_mc.xtd', '{IsDebug:1,Meta:{RandomSeed:test, IsMultiPredict:1}}', 'multifeature_winloss_mc_factors.tsv'],
    ['multifeature_winloss_mc_with_addition_model', 'winloss_mc_with_addition_model.xtd', STUB_CTX, 'multifeature_winloss_mc_with_combination_boost.tsv'],
    ['multifeature_winloss_mc_with_addition_model_with_remain_position_heuristics', 'winloss_mc_with_addition_model_remain_position_heuristics.xtd', STUB_CTX, 'multifeature_winloss_mc_with_combination_boost.tsv'],  # noqa: E501
    ['multifeature_winloss_mc_with_addition_model_recalc', 'winloss_mc_with_addition_model.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:all}}', 'multifeature_winloss_mc_with_combination_boost.tsv', '{"AdditionModelParams": {"AdditionScoreCoef": 123}}'],  # noqa: E501
    ['multifeature_winloss_mc_with_max_win', 'multifeature_winloss_mc_with_max_win.xtd', STUB_CTX, 'multifeature_winloss_mc_factors.tsv'],
    ['multifeature_winloss_mc_with_unshifted_threshold', 'multifeature_winloss_mc_with_unshifted_threshold.xtd', STUB_CTX, 'multifeature_winloss_mc_factors.tsv'],
    ['winloss_mc_with_win_subtarget_index', 'winloss_mc_with_win_subtarget_index.xtd', STUB_CTX, 'multifeature_winloss_mc_factors.tsv'],
    ['winloss_mc_best_scores_passing', 'winloss_mc_with_win_subtarget_index.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:best}}', 'multifeature_winloss_mc_factors.tsv'],
    ['winloss_mc_best_all_passing', 'winloss_mc_with_win_subtarget_index.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:all}}', 'multifeature_winloss_mc_factors.tsv'],
    ['winloss_mc_best_all_passing_patch_cb04', 'winloss_mc_with_win_subtarget_index.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:all, CalcerPatch:{multifeature_winloss_mc:{Params:{ClickBoost:0.4}}}}}', 'multifeature_winloss_mc_factors.tsv'],  # noqa: E501
    ['winloss_mc_best_all_passing_recalc', 'winloss_mc_with_win_subtarget_index.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:all}}', 'multifeature_winloss_mc_factors.tsv', '{"Params": {"ShowScoreThreshold": 100}}'],  # noqa: E501
    ['winloss_mc_best_all_passing_recalc_changeless', 'winloss_mc_with_win_subtarget_index.xtd', '{IsDebug:1,Meta:{RandomSeed:test, PassRawPredictions:all}}', 'multifeature_winloss_mc_factors.tsv', '{}'],  # noqa: E501
    ['thompson_sampling', 'thompson_sampling_wd_10_sst_0.7.xtd', '{IsDebug:1,Meta:{RandomSeed:test,PredictedPosInfo:5}}', 'thompson_sampling_factors.tsv'],
    ['multifeature_winloss_mc_with_position_filter', 'multifeature_winloss_mc_with_position_filter.xtd', STUB_CTX, 'multifeature_winloss_mc_factors.tsv'],
    ['thompson_sampling_win_loss_beta', 'thompson_sampling_win_loss_beta_wd_10_sst_0.7_up_0.3.xtd', '{IsDebug:1,Meta:{RandomSeed:joke,PredictedPosInfo:5}}', 'thompson_sampling_factors.tsv'],
    ['thompson_sampling_win_loss_shows', 'thompson_sampling_win_loss_shows_wd_10_sst_0.7_up_0.3.xtd', '{IsDebug:1,Meta:{RandomSeed:joke,PredictedPosInfo:5}}', 'thompson_sampling_factors.tsv'],
    ['thompson_sampling_from_subtarget', 'thompson_sampling_from_subtarget_sm_0.1_sa_100.xtd', CONTEXT_WITH_SUB_SCORES, 'thompson_sampling_factors.tsv'],
    [
        'locrandom_show_proba_multiplier_if_filtered', 'locrandom_show_proba_multiplier_if_filtered.xtd',
        '{IsDebug:1,Meta:{RandomSeed:test,PredictedFeaturesInfo:{IsFiltered:true}}}', 'factors.tsv'
    ],
    ['thompson_sampling_use_total_counters_as_shows', 'thompson_sampling_use_total_counters_as_shows.xtd', CONTEXT_WITH_SUB_SCORES, 'thompson_sampling_factors.tsv'],
    ['winloss_mc_without_position_feature', 'winloss_mc_without_position_feature.xtd', STUB_CTX, 'winloss_mc_without_position_factors.tsv'],
    ['thompson_sampling_fallback_test', 'thompson_sampling_use_total_counters_as_shows.xtd', STUB_CTX, 'thompson_sampling_factors.tsv'],
    ['winloss_mc_with_bundle_info', 'multifeature_winloss_mc.xtd', '{IsDebug:1,Meta:{RandomSeed:test,AddBundleInfo:1}}', 'multifeature_winloss_mc_factors.tsv'],
    # ['two_calcers_union', 'two_calcers_union.xtd', STUB_CTX, 'factors.tsv']  # No such resource
]

names = [d[0] for d in test_data]


@pytest.mark.parametrize("data", test_data, ids=names)
def test_click_pool(data):
    args = [
        '-f', data[1],
        '-s', common.source_path(os.path.join("kernel/extended_mx_calcer/tools/calcers_test/tests/data", data[3])),
        '-c', data[2],
    ]
    if len(data) == 5:
        args.extend(['-r', data[4]])

    return common.canonical_execute(
        common.binary_path("kernel/extended_mx_calcer/tools/calcers_test/calcers_test"),
        args,
        file_name=data[0]
    )

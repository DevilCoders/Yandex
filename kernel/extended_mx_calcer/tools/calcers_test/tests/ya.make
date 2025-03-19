OWNER(epar)

PY2TEST()

SIZE(LARGE)

TAG(ya:fat)

TEST_SRCS(test_extended_calcer.py)

DATA(
    sbr://96087133   # always_2.xtd
    sbr://96089011   # bad_clickint.xtd
    sbr://96089038   # bad_meta.xtd
    sbr://96089058   # clickint_add_rand.xtd
    sbr://96089083   # clickint_random.xtd
    sbr://96089103   # clickregtree.xtd
    sbr://96089128   # mc_with_filter.xtd
    sbr://96089155   # multi_clickint.xtd
    sbr://96089175   # simple_meta.xtd
    sbr://96089201   # spwll.xtd
    sbr://96089242   # win_loss.xtd
    sbr://96091410   # multitarget.xtd
    sbr://96091586   # multitarget_fast.xtd
    sbr://772158317  # positional_surplus.xtd
    sbr://97409460   # multi_sam.xtd
    sbr://110078503  # images_swifty_30bin_upper.xtd
    sbr://110782229  # pos_bin_composition.xtd
    sbr://110753297  # positional_random.xtd
    sbr://114352866  # clickregtree_stump.xtd
    sbr://114801484  # positional_perceptron.xtd
    sbr://114801639  # positional_bayes.xtd
    sbr://116978052  # multitarget_with_res_feats.xtd
    sbr://130404721  # one_position_binary.xtd
    sbr://177180231  # images_azazello_stub.xtd
    sbr://139653344  # video_azazello_test_bin.xtd
    sbr://139649060  # video_azazello_test_int.xtd
    sbr://139648987  # video_azazello_test_binint.xtd
    sbr://390564881  # video_azazello_top_test_binint.xtd
    sbr://149549895  # view_place_pos_random.xtd
    sbr://149859731  # view_place_pos_random_with_noshow.xtd
    sbr://150159722  # clickint_incut_mix.xtd
    sbr://150159858  # random_incut_mix.xtd
    sbr://150160082  # random_mask_mix.xtd
    sbr://157611351  # multibundle_by_factors.xtd
    sbr://162108871  # binary_with_arbitary_result.xtd
    sbr://181103478  # weight_local_random.xtd
    sbr://248076057  # cascade.xtd
    sbr://270341369  # factor_filter.xtd
    sbr://347934992  # multifeature_winloss_mc.xtd
    sbr://528596259  # mx_with_meta.xtd
    sbr://618633349  # winloss_mc_with_relev_boost_test_bundle.xtd
    sbr://740479272  # winloss_mc_with_combination_boost.xtd
    sbr://754159759  # bundle_with_catfeatures.xtd
    sbr://813633048  # alwyas_one_with_random.xtd
    sbr://935643650  # market_model_with_noshows.xtd
    sbr://1004970360 # winloss_mc_with_addition_model.xtd
    sbr://1101474527 # winloss_mc_with_addition_model_remain_position_heuristics.xtd
    sbr://1176935800 # multifeature_winloss_mc_with_max_win.xtd
    sbr://1176939718 # multifeature_winloss_mc_with_unshifted_threshold.xtd
    sbr://1179368441 # winloss_mc_with_win_subtarget_index.xtd
    sbr://1357855189 # thompson_sampling_wd_10_sst_0.7.xtd
    sbr://1362148305 # multifeature_winloss_mc_with_position_filter.xtd
    sbr://1370570798 # thompson_sampling_win_loss_beta_wd_10_sst_0.7_up_0.3.xtd
    sbr://1377933905 # thompson_sampling_win_loss_shows_wd_10_sst_0.7_up_0.3.xtd
    sbr://1514893330 # thompson_sampling_from_subtarget_sm_0.1_sa_100.xtd
    sbr://1550047304 # locrandom_show_proba_multiplier_if_filtered.xtd
    sbr://1661773717 # thompson_sampling_use_total_counters_as_shows.xtd
    sbr://1637974670 # winloss_mc_without_position_feature.xtd
    # sbr://2001946750 # two_calcers_union.xtd  No such file in Sandbox
)

DEPENDS(kernel/extended_mx_calcer/tools/calcers_test)



END()

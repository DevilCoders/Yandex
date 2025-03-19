PY2TEST()

OWNER(
    insight
    g:neural-search
)

TEST_SRCS(
    test.py
)

DATA(
    # search_models.appl
    sbr://222148983

    # ctrserp_click_vs_noclickeasy_normquant256bin_128bit
    sbr://200807904

    # ctrserp_click_vs_noclickhard_em300_normquant16bin_128bit_annealed
    sbr://208108099

    # rsya_ctr50.appl
    sbr://1063609024

    # jruziev_dssmtsar2_ctr_bc_50_2l.appl
    sbr://1108551748

    # jruziev_tsar_searchBC1.appl
    sbr://1243166002

    arcadia_tests_data/dssm
)

DEPENDS(
    kernel/dssm_applier/nn_applier
)

SIZE(LARGE)

TAG(ya:fat)

END()

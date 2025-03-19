PY2TEST()

OWNER(
    g:neural-search
    alexbykov
)

SIZE(MEDIUM)

TEST_SRCS(
    test.py
)

DEPENDS(
    kernel/dssm_applier/optimized_model/bench
)

DATA(
    sbr://3191574112 #optimized_model.dssm
    sbr://1165246410 #optimized_dssm_model_bench_input
)

REQUIREMENTS(ram:10)

END()

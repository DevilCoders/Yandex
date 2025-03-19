EXECTEST()

SIZE(MEDIUM)

OWNER(
    g:neural-search
    alexbykov
)

RUN(
    NAME optimized_model_apply
    test
        --mode Apply
    STDOUT apply_results.txt
    CANONIZE apply_results.txt
)

RUN(
    NAME optimized_model_apply_all
    test
        --mode ApplyAll
    STDOUT apply_all_results.txt
    CANONIZE apply_all_results.txt
)

DEPENDS(
    kernel/dssm_applier/optimized_model/test
)

DATA(
    sbr://3191574112 #optimized_model.dssm
    sbr://1165246410 #optimized_dssm_model_bench_input
)

END()
